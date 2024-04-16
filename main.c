#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdbool.h>

#include "utils.c"
#include "client.c"

volatile int running;
volatile int server_sockfd;

int tcp_socket_bind(int sockfd, uint16_t port) {
    assert(port >= 1024 && port <= 49151);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    return bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
}

int tcp_socket_accept(int sockfd) {
    // We don't care about the client address
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    return accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
}

String read_request(int sockfd) {
    size_t buffer_size = 8192;
    char *buffer = calloc(1, buffer_size);
    int n = read(sockfd, buffer, buffer_size - 1);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    return str_new(buffer);
}

typedef struct {
    size_t len;
    String *headers;
} Headers;

void print_headers(Headers headers) {
    printf("Headers:\n");
    for (size_t i = 0; i < headers.len; i++) {
        printf("  %s\n", headers.headers[i].str);
    }
}

typedef struct {
    String method;
    String path;
    String version;
    String body;
    Headers headers;
} Request;

void print_request(Request req) {
    printf("Request:\n");
    if (req.method.str == NULL) {
        return;
    }
    printf("Method: %s\n", req.method.str);
    if (req.path.str == NULL) {
        return;
    }
    printf("Path: %s\n", req.path.str);
    if (req.version.str == NULL) {
        return;
    }
    printf("Version: %s\n", req.version.str);
    if (req.body.str == NULL) {
        return;
    }
    printf("Body: %s\n", req.body.str);
    print_headers(req.headers);
}

Request parse_request(String request) {
    char *token = NULL;
    char *str = strdup(request.str);
    char *strfree = str;
    Request req = {0};
    token = strsep(&str, " ");
    if (token == NULL) {
        return req;
    }
    req.method = str_new(strdup(token));

    token = strsep(&str, " ");
    if (token == NULL) {
        return req;
    }
    req.path = str_new(strdup(token));

    token = strsep(&str, "\n");
    if (token == NULL) {
        return req;
    }
    req.version = str_new(strdup(token));

    Headers headers = {0, NULL};
    token = strsep(&str, "\n");
    while (token != NULL && strncmp(token, "\r", 1) != 0) {
        if (headers.len == 0) {
            headers.headers = calloc(1, sizeof(String));
        } else {
            headers.headers = realloc(headers.headers, (headers.len + 1) * sizeof(String));
        }
        headers.len += 1;
        headers.headers[headers.len - 1] = str_new(strdup(token));
        token = strsep(&str, "\n");
    }
    if (token == NULL) {
        return req;
    }
    req.headers = headers;

    char *body = calloc(1, 1);
    size_t body_len = 1;
    token = strsep(&str, "\n");
    while (token != NULL) {
        body_len = body_len + strlen(token);
        body = realloc(body, body_len);
        strlcat(body, token, body_len);
        token = strsep(&str, "\n ");
    }
    req.body = str_new(body);
    printf("body_len: %zu\n", body_len);

    free(strfree);
    return req;
}

void request_drop(Request req) {
    str_drop(req.method);
    str_drop(req.path);
    str_drop(req.version);
    str_drop(req.body);
    for (size_t i = 0; i < req.headers.len; i++) {
        str_drop(req.headers.headers[i]);
    }
    free(req.headers.headers);
}

String handle_request(Request request) {
    return str_new("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!");
}

typedef struct {
    int port;
} ServerArgs;

void *run_server(void *args) {
    ServerArgs *server_args = (ServerArgs *)args;
    server_sockfd = tcp_socket_new();
    if (server_sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Allow the server to bind to the same port, overriding the TIME_WAIT
    // that by default doesn't allow binding to the same port for a few
    // minutes.
    int yes = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("ERROR setsockopt SO_REUSEADDR");
        close(server_sockfd);
        exit(1);
    }

    int port = server_args->port;
    if (tcp_socket_bind(server_sockfd, port) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    int backlog_size = 16;
    listen(server_sockfd, backlog_size);

    while (running) {
        int newsockfd = tcp_socket_accept(server_sockfd);
        if (newsockfd < 0) {
            if (running == 0) {
                printf("Server shutting down...\n");
                break;
            }
            perror("ERROR on accept");
            exit(1);
        }

        String msg = read_request(newsockfd);
        printf("Received message:\n%s\n", msg.str);

        Request req = parse_request(msg);
        print_request(req);

        String response = handle_request(req);
        if (write(newsockfd, response.str, str_len(response)) < 0) {
            perror("ERROR writing to socket");
            exit(1);
        }

        str_drop(msg);
        request_drop(req);
        close(newsockfd);
    }

    close(server_sockfd);
    pthread_exit(NULL);
}

void shutdown_server() {
    running = 0;
    close(server_sockfd);
    shutdown(server_sockfd, SHUT_RDWR);
}

void sigint_handler(int sig) {
    printf("Received SIGINT, shutting down...\n");
    shutdown_server();
}

int main()
{
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("error setting signal handler");
        exit(1);
    }

    int port = 8080;
    pthread_t server_thread;
    ServerArgs server_args = {port};
    running = 1;
    pthread_create(&server_thread, NULL, run_server, &server_args);

    pthread_t client_thread;
    ClientArgs client_args = {"localhost", port};
    pthread_create(&client_thread, NULL, run_client, &client_args);
    pthread_join(client_thread, NULL);
    printf("Client thread joined\n");

    pthread_join(server_thread, NULL);

    return 0;
}

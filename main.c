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
#include "http.c"

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

typedef struct {
    String name;
    int passed;
} TestResult;

TestResult test_parse_http_request() {
    String request = str_new("GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
    Request req = parse_request(request);
    assert(strncmp(req.method.str, "GET", 3) == 0);
    assert(strncmp(req.path.str, "/", 1) == 0);
    assert(strncmp(req.version.str, "HTTP/1.1", 8) == 0);
    assert(req.headers.len == 1);
    assert(strncmp(req.headers.headers[0].str, "Host: localhost:8080", 19) == 0);
    assert(strncmp(req.body.str, "", 1) == 0);
    request_drop(req);
    return (TestResult){str_new("test_parse_http_request"), 1};
}

void run_tests() {
    const size_t n_tests = 1;
    TestResult results[n_tests] = {
        test_parse_http_request(),
    };
    for (size_t i = 0; i < n_tests; i++) {
        TestResult result = results[i];
        if (result.passed) {
            printf("PASSED: %s\n", result.name.str);
        } else {
            printf("FAILED: %s\n", result.name.str);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strncmp(argv[1], "test", 4) == 0) {
        run_tests();
        return 0;
    }

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

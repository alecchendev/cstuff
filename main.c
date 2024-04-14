#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdbool.h>

volatile int running;
volatile int server_sockfd;

// Simple wrapper for heap-allocated char*
// with a length.
typedef struct {
    char *str;
    size_t len;
} String;

String str_new(char *str)
{
    String s = {str, strnlen(str, INT_MAX)};
    return s;
}

size_t str_len(String s)
{
    return s.len;
}

void str_drop(String s)
{
    free(s.str);
}

int tcp_socket_new() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

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

String handle_request(String request) {
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

        String request = read_request(newsockfd);
        printf("Received request:\n%s\n", request.str);

        String response = handle_request(request);
        if (write(newsockfd, response.str, str_len(response)) < 0) {
            perror("ERROR writing to socket");
            exit(1);
        }

        str_drop(request);
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

struct sockaddr_in get_ipv4_sockaddr(char *host, int port) {
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    return serv_addr;
}

String read_response(int sockfd) {
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
    char *host;
    int port;
} ClientArgs;

void *run_client(void *args) {
    ClientArgs *client_args = (ClientArgs *)args;

    struct sockaddr_in serv_addr = get_ipv4_sockaddr(client_args->host, client_args->port);

    int sockfd = tcp_socket_new();
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    String request = str_new("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");

    printf("Sending request:\n%s\n", request.str);

    if (write(sockfd, request.str, str_len(request)) < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    String response = read_response(sockfd);

    printf("Received response:\n%s\n", response.str);

    pthread_exit(NULL);
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

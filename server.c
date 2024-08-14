#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#include "utils.c"
#include "http.c"

volatile int running;
volatile int server_sockfd;

void shutdown_server() {
    running = 0;
    close(server_sockfd);
    shutdown(server_sockfd, SHUT_RDWR);
}

void sigint_handler(int sig) {
    printf("Received SIGINT, shutting down...\n");
    shutdown_server();
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

String handle_request(Request request) {
    return str_new("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!");
}

typedef struct {
    // The file descriptor for the socket the server is listening on.
    int sockfd;
} ServerArgs;

void *accept_and_handle_requests(void *args) {
    ServerArgs *server_args = (ServerArgs *)args;
    int sockfd = server_args->sockfd;
    while (running) {
        int newsockfd = tcp_socket_accept(sockfd);
        if (newsockfd < 0) {
            if (running == 0) {
                printf("Server shutting down...\n");
                break;
            }
            perror("ERROR on accept");
            exit(1);
        }

        String msg = read_request(newsockfd);
        Request req = parse_request(msg);

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

pthread_t start_server(int port) {
    // Setup for graceful shutdown
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("error setting signal handler");
        exit(1);
    }

    running = 1;

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

    printf("Starting server on port %d\n", port);
    if (tcp_socket_bind(server_sockfd, port) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    int backlog_size = 16;
    listen(server_sockfd, backlog_size);

    ServerArgs *server_args = calloc(1, sizeof(ServerArgs));
    server_args->sockfd = server_sockfd;

    pthread_t server_thread;
    int res = pthread_create(&server_thread, NULL, accept_and_handle_requests, server_args);
    if (res != 0) {
        perror("ERROR creating server thread");
        exit(1);
    }
    return server_thread;
}


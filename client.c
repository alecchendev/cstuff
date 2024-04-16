#pragma once
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

    const char *http_post_request =
    "POST /data HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 81\r\n"
    "\r\n"
    "{\n"
    "  \"username\": \"testuser\",\n"
    "  \"password\": \"testpassword\"\n"
    "}\n";

    String request = str_new((char *)http_post_request);

    printf("Sending request:\n%s\n", request.str);

    if (write(sockfd, request.str, str_len(request)) < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    String response = read_response(sockfd);

    printf("Received response:\n%s\n", response.str);

    pthread_exit(NULL);
}

#pragma once
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

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
    if (s.str != NULL) {
        free(s.str);
    }
}

// Create a new TCP socket, returns the file descriptor.
int tcp_socket_new() {
    return socket(AF_INET, SOCK_STREAM, 0);
}


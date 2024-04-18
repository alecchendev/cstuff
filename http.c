#pragma once
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "utils.c"
#include "test_utils.c"

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

typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    ERROR,
} Method;

Method str_to_method(const char *str) {
    if (strncmp(str, "GET", 3) == 0) {
        return GET;
    } else if (strncmp(str, "POST", 4) == 0) {
        return POST;
    } else if (strncmp(str, "PUT", 3) == 0) {
        return PUT;
    } else if (strncmp(str, "DELETE", 6) == 0) {
        return DELETE;
    }
    return ERROR;
}

// No heap allocation!
String method_to_str(Method method) {
    switch (method) {
    case GET:
        return str_new("GET");
    case POST:
        return str_new("POST");
    case PUT:
        return str_new("PUT");
    case DELETE:
        return str_new("DELETE");
    default:
        return str_new("ERROR");
    }
}

typedef struct {
    Method method;
    String path;
    String version;
    String body;
    Headers headers;
} Request;

void print_request(Request req) {
    printf("Request:\n");
    char *method = method_to_str(req.method).str;
    if (method == NULL) {
        return;
    }
    printf("Method: %s\n", method);
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
    Method method = str_to_method(token);
    if (method == ERROR) {
        printf("Invalid method: %s\n", token);
        return req;
    }
    req.method = method;

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

    free(strfree);
    return req;
}

void request_drop(Request req) {
    str_drop(req.path);
    str_drop(req.version);
    str_drop(req.body);
    for (size_t i = 0; i < req.headers.len; i++) {
        str_drop(req.headers.headers[i]);
    }
    free(req.headers.headers);
}

TestResult test_parse_http_request() {
    String request = str_new("GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
    Request req = parse_request(request);
    assert(req.method == GET);
    assert(strncmp(req.path.str, "/", 1) == 0);
    assert(strncmp(req.version.str, "HTTP/1.1", 8) == 0);
    assert(req.headers.len == 1);
    assert(strncmp(req.headers.headers[0].str, "Host: localhost:8080", 19) == 0);
    assert(strncmp(req.body.str, "", 1) == 0);
    request_drop(req);
    return (TestResult){"test_parse_http_request", 1};
}

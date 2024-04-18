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
#include "server.c"
#include "client.c"
#include "http.c"
#include "test_utils.c"


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

    int port = 8080;
    pthread_t server_thread = start_server(port);

    pthread_t client_thread;
    ClientArgs client_args = {"localhost", port};
    pthread_create(&client_thread, NULL, run_client, &client_args);
    pthread_join(client_thread, NULL);
    printf("Client thread joined\n");

    pthread_join(server_thread, NULL);

    return 0;
}

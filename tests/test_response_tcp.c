#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../client/response.h"
#include "../client/state.h"
#include "../common/protocol.h"

#define TEST_PORT 5555

/*
 * Starts a fake TCP server using the same socket primitives used
 * in the real project.
 */
static void start_fake_server(void) {
    int server_fd;
    int client_fd;
    int opt;
    struct sockaddr_in addr;
    char response[MAX_MSG * 4];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        exit(1);
    }

    snprintf(response, sizeof(response),
             "MAP LOCAL 3 3\n"
             "###\n"
             "#P#\n"
             "###\n"
             "END\n");

    write(client_fd, response, strlen(response));

    close(client_fd);
    close(server_fd);
}

/*
 * Connects to the fake TCP server and verifies that response.c
 * correctly parses the received MAP message.
 */
static int run_client_test(void) {
    int sock;
    struct sockaddr_in addr;
    char first_line[MAX_MSG];
    ClientState state;

    init_client_state(&state);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sleep(1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 0;
    }

    if (recv_line(sock, first_line, sizeof(first_line)) < 0) {
        close(sock);
        return 0;
    }

    if (handle_server_message(sock, first_line, &state) < 0) {
        close(sock);
        return 0;
    }

    close(sock);
    return 1;
}

/*
 * Test entry point.
 */
int main(void) {
    pid_t pid;
    int result;

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        start_fake_server();
        exit(0);
    }

    result = run_client_test();

    if (result) {
        printf("PASSED: TCP response parser test\n");
        return 0;
    }

    printf("FAILED: TCP response parser test\n");
    return 1;
}

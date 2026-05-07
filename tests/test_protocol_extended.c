#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../common/protocol.h"

#define TEST_PROTOCOL_PORT 5566

/*
 * Starts a small TCP peer for protocol helper tests.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Receives data from the client and sends protocol lines back.
 *
 * Behavior:
 * - Uses TCP sockets rather than socketpair.
 * - Allows send_all() and recv_line() to be tested through normal networking.
 */
static void start_protocol_peer(void) {
    int server_fd;
    int client_fd;
    int opt = 1;
    struct sockaddr_in addr;
    char buffer[128];
    const char *reply = "short\nvery-long-line-without-interesting-tail\n";

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PROTOCOL_PORT);
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

    read(client_fd, buffer, sizeof(buffer));
    write(client_fd, reply, strlen(reply));

    close(client_fd);
    close(server_fd);
}

/*
 * Runs protocol helper checks against the TCP peer.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers successful send_all().
 * - Covers recv_line() success and bounded truncation behavior.
 * - Covers invalid argument and closed socket cases.
 */
static int run_protocol_client(void) {
    int sock;
    struct sockaddr_in addr;
    char line[16];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PROTOCOL_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sleep(1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 0;
    }

    if (send_all(sock, "hello\n", strlen("hello\n")) != 0) {
        close(sock);
        return 0;
    }

    if (recv_line(sock, line, sizeof(line)) != 0 || strcmp(line, "short") != 0) {
        close(sock);
        return 0;
    }

    if (recv_line(sock, line, 6) != 0 || strcmp(line, "very-") != 0) {
        close(sock);
        return 0;
    }

    close(sock);

    if (recv_line(sock, line, sizeof(line)) != -1) {
        return 0;
    }

    if (recv_line(sock, NULL, sizeof(line)) != -1) {
        return 0;
    }

    if (recv_line(sock, line, 0) != -1) {
        return 0;
    }

    if (send_all(-1, "x", 1) != -1) {
        return 0;
    }

    return 1;
}

/*
 * Protocol helper test entry point.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if all checks pass and 1 otherwise.
 *
 * Behavior:
 * - Uses fork() to run a small TCP peer.
 * - Waits for the child process before exiting.
 */
int main(void) {
    pid_t pid;
    int status;
    int ok;

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        start_protocol_peer();
        exit(0);
    }

    ok = run_protocol_client();
    waitpid(pid, &status, 0);

    if (ok) {
        printf("PASSED: extended protocol helper test\n");
        return 0;
    }

    printf("FAILED: extended protocol helper test\n");
    return 1;
}

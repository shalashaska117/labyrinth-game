#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../client/response.h"
#include "../client/state.h"
#include "../common/constants.h"
#include "../common/protocol.h"

#define TEST_PORT 5555

/*
 * Starts a fake TCP server for response parser testing.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Sends one MAP message and one SESSION LOBBY message to a test client.
 *
 * Behavior:
 * - Uses the same socket primitives as the project.
 * - Avoids external testing frameworks.
 * - Keeps the server alive long enough for the client to parse both messages.
 */
static void start_fake_server(void) {
    int server_fd;
    int client_fd;
    int opt;
    struct sockaddr_in addr;
    char response[MAX_MSG * 4];
    char client_line[MAX_MSG];

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
             "END\n"
             "SESSION LOBBY\n");

    write(client_fd, response, strlen(response));

    /*
     * SESSION LOBBY makes the client send LIST automatically.
     * Read it if it arrives, but do not fail the fake server if the client closes first.
     */
    recv_line(client_fd, client_line, sizeof(client_line));

    close(client_fd);
    close(server_fd);
}

/*
 * Connects to the fake server and parses its responses.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 if parsing succeeds and 0 otherwise.
 *
 * Behavior:
 * - Reads the first MAP line with recv_line().
 * - Delegates full parsing to response.c.
 * - Verifies both map storage and SESSION LOBBY handling.
 */
static int run_client_test(void) {
    int sock;
    struct sockaddr_in addr;
    char first_line[MAX_MSG];
    ClientState state;
    int map_ok;
    int lobby_ok;

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

    map_ok = (state.local_map[4] == CELL_PLAYER);

    if (recv_line(sock, first_line, sizeof(first_line)) < 0) {
        close(sock);
        return 0;
    }

    if (handle_server_message(sock, first_line, &state) < 0) {
        close(sock);
        return 0;
    }

    lobby_ok = (state.session_state == CLIENT_LOBBY &&
                state.mode == COMMAND &&
                state.show_global == 0 &&
                state.rank_count == 0);

    close(sock);
    return map_ok && lobby_ok;
}

/*
 * Test entry point.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if the parser test passes and 1 otherwise.
 *
 * Behavior:
 * - Uses fork() to run a small fake server and test client.
 * - Waits for the child process to avoid leaving a zombie process.
 */
int main(void) {
    pid_t pid;
    int result;
    int status;

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
    waitpid(pid, &status, 0);

    if (result) {
        printf("PASSED: TCP response parser test\n");
        return 0;
    }

    printf("FAILED: TCP response parser test\n");
    return 1;
}
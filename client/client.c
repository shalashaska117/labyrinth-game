#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>

#include "../common/protocol.h"

#include "ui.h"
#include "command.h"
#include "response.h"
#include "state.h"

/*
 * Removes the final newline from a string, if present.
 *
 * str: string to clean.
 */
static void remove_newline(char *str) {
    size_t len = strlen(str);

    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

/*
 * Creates a TCP connection to the server.
 *
 * host: server hostname or IP address.
 * port: server port as a string.
 *
 * getaddrinfo() is used because it supports both numeric IP addresses
 * and hostnames such as the Docker Compose service name "server".
 *
 * Returns a connected socket descriptor on success, -1 on error.
 */
static int connect_to_server(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int sock;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &result) != 0) {
        return -1;
    }

    sock = -1;

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sock < 0) {
            continue;
        }

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(result);

    return sock;
}

/*
 * Checks whether a command requires authentication.
 *
 * input: command typed by the user.
 *
 * Returns 1 if the command requires login, 0 otherwise.
 */
static int requires_login(const char *input) {
    if (strncmp(input, "login ", 6) == 0) {
        return 0;
    }

    if (strncmp(input, "register ", 9) == 0) {
        return 0;
    }

    if (strcmp(input, "help") == 0) {
        return 0;
    }

    if (strcmp(input, "quit") == 0) {
        return 0;
    }

    return 1;
}

/*
 * Extracts the nickname from login/register commands.
 *
 * input: command typed by the user.
 * nickname: destination buffer.
 * size: size of nickname buffer.
 *
 * Returns 1 if a nickname was extracted, 0 otherwise.
 */
static int extract_auth_nickname(const char *input, char *nickname, size_t size) {
    char temp[MAX_NICK];
    char password[MAX_PASSWORD];

    if (sscanf(input, "login %31s %63s", temp, password) == 2) {
        snprintf(nickname, size, "%s", temp);
        return 1;
    }

    if (sscanf(input, "register %31s %63s", temp, password) == 2) {
        snprintf(nickname, size, "%s", temp);
        return 1;
    }

    return 0;
}

/*
 * Main client logic.
 *
 * The client connects to a TCP server and then uses select() to monitor
 * both standard input and the socket connected to the server.
 *
 * argv[1]: server IP address or hostname.
 * argv[2]: server port.
 *
 * Returns 0 on normal termination, 1 on startup error.
 */
int main(int argc, char *argv[]) {
    int sock;
    fd_set readfds;
    char input[MAX_MSG];
    char command[MAX_MSG];
    char first_line[MAX_MSG];
    char auth_nickname[MAX_NICK];
    ClientState state;

    if (argc != 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }

    init_client_state(&state);

    sock = connect_to_server(argv[1], argv[2]);

    if (sock < 0) {
        printf("Unable to connect to server %s:%s\n", argv[1], argv[2]);
        return 1;
    }

    printf("Connected to server %s:%s\n", argv[1], argv[2]);
    print_help();

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        int maxfd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;
            }

            remove_newline(input);

            if (!state.logged_in && requires_login(input)) {
                printf("You must login or register before playing.\n");
                continue;
            }

            int result = build_protocol_command(input, command, sizeof(command));

            if (result == 0) {
                continue;
            }

            if (result < 0) {
                printf("Invalid command. Type 'help' to show available commands.\n");
                continue;
            }

            if (extract_auth_nickname(input, auth_nickname, sizeof(auth_nickname))) {
                set_pending_auth(&state, auth_nickname);
            }

            if (send_all(sock, command, strlen(command)) < 0) {
                printf("Send error\n");
                break;
            }

            if (strcmp(input, "quit") == 0) {
                printf("Disconnecting...\n");
                break;
            }
        }

        if (FD_ISSET(sock, &readfds)) {
            if (recv_line(sock, first_line, sizeof(first_line)) < 0) {
                printf("Disconnected from server\n");
                break;
            }

            if (handle_server_message(sock, first_line, &state) < 0) {
                printf("Protocol error or disconnected server.\n");
                break;
            }
        }
    }

    close(sock);
    return 0;
}

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/protocol.h"

#include "command.h"
#include "response.h"
#include "state.h"
#include "ui.h"

/*
 * Creates a TCP connection to the server.
 *
 * Input:
 * - host: server hostname or IPv4 address.
 * - port: server port as a string.
 *
 * Output:
 * - Returns a connected socket descriptor on success.
 * - Returns -1 on connection or resolution error.
 *
 * Behavior:
 * - Uses getaddrinfo() to support numeric addresses and Docker service names.
 * - Tries each resolved address until connect() succeeds.
 */
static int connect_to_server(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &result) != 0) {
        return -1;
    }

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
 * Checks whether a command can be sent before authentication.
 *
 * Input:
 * - input: command typed by the user.
 *
 * Output:
 * - Returns 1 if authentication is required.
 * - Returns 0 if the command is allowed before authentication.
 *
 * Behavior:
 * - Allows login, register, help and quit before authentication.
 * - Accepts slash-prefixed variants in command mode.
 * - Prevents gameplay/session commands before the user is authenticated.
 */
static int requires_login(const char *input) {
    const char *cmd = input;

    if (cmd == NULL) {
        return 1;
    }

    if (cmd[0] == '/') {
        cmd++;
    }

    if (strncmp(cmd, "login ", 6) == 0) {
        return 0;
    }

    if (strncmp(cmd, "register ", 9) == 0) {
        return 0;
    }

    if (strcmp(cmd, "help") == 0) {
        return 0;
    }

    if (strcmp(cmd, "quit") == 0) {
        return 0;
    }

    return 1;
}

/*
 * Extracts the nickname from login or register commands.
 *
 * Input:
 * - input: command typed by the user.
 * - nickname: destination buffer.
 * - size: size of the destination buffer.
 *
 * Output:
 * - Returns 1 if a nickname is found, 0 otherwise.
 * - Writes the extracted nickname when successful.
 *
 * Behavior:
 * - Accepts both normal and slash-prefixed commands.
 */
static int extract_auth_nickname(const char *input, char *nickname, size_t size) {
    char temp[MAX_NICK];
    char password[MAX_PASSWORD];
    const char *cmd = input;

    if (cmd == NULL || nickname == NULL || size == 0) {
        return 0;
    }

    if (cmd[0] == '/') {
        cmd++;
    }

    if (sscanf(cmd, "login %31s %63s", temp, password) == 2) {
        snprintf(nickname, size, "%s", temp);
        return 1;
    }

    if (sscanf(cmd, "register %31s %63s", temp, password) == 2) {
        snprintf(nickname, size, "%s", temp);
        return 1;
    }

    return 0;
}

/*
 * Updates the map overlay flag before sending map-related commands.
 *
 * Input:
 * - state: current client state.
 * - input: command typed by the user.
 *
 * Output:
 * - Updates state->show_global when the command explicitly requests LOCAL or GLOBAL.
 *
 * Behavior:
 * - LOCAL disables the global overlay.
 * - GLOBAL enables the global overlay.
 * - Slash-prefixed variants are handled as equivalent commands.
 */
static void update_overlay_from_command(ClientState *state, const char *input) {
    const char *cmd = input;

    if (state == NULL || input == NULL) {
        return;
    }

    if (cmd[0] == '/') {
        cmd++;
    }

    if (strcmp(cmd, "local") == 0) {
        state->show_global = 0;
        return;
    }

    if (strcmp(cmd, "global") == 0) {
        state->show_global = 1;
        return;
    }
}

/*
 * Sends a user command after translating it to the protocol.
 *
 * Input:
 * - sock: connected socket descriptor.
 * - state: current client state.
 * - input: command string to send.
 *
 * Output:
 * - Returns 0 on success and -1 on send error.
 * - May update pending authentication state and map overlay state.
 *
 * Behavior:
 * - Rejects protected commands before authentication.
 * - Uses command.c for all user-to-protocol translation.
 * - Handles local commands such as help without sending data.
 * - Keeps the local/global overlay flag synchronized with explicit map commands.
 */
static int send_user_command(int sock, ClientState *state, const char *input) {
    char command[MAX_MSG];
    char auth_nickname[MAX_NICK];
    int result;

    if (state == NULL || input == NULL || input[0] == '\0') {
        return 0;
    }

    if (!state->logged_in && requires_login(input)) {
        state_set_status(state, "You must login or register before playing.");
        draw_screen(state);
        return 0;
    }

    result = build_protocol_command(input, command, sizeof(command));

    if (result == 0) {
        draw_screen(state);
        return 0;
    }

    if (result < 0) {
        state_set_status(state, "Invalid command. Type /help to show commands.");
        draw_screen(state);
        return 0;
    }

    if (extract_auth_nickname(input, auth_nickname, sizeof(auth_nickname))) {
        set_pending_auth(state, auth_nickname);
    }

    update_overlay_from_command(state, input);

    if (send_all(sock, command, strlen(command)) < 0) {
        return -1;
    }

    return 0;
}

/*
 * Sends an immediate movement command from movement mode.
 *
 * Input:
 * - sock: connected socket descriptor.
 * - dir: direction key typed by the user.
 *
 * Output:
 * - Returns 0 on success and -1 on send error.
 *
 * Behavior:
 * - Maps w, a, s and d to MOVE commands.
 * - Does not wait for ENTER.
 */
static int send_movement_key(int sock, char dir) {
    char command[MAX_MSG];
    const char *protocol_dir;

    switch (dir) {
        case 'w':
        case 'W':
            protocol_dir = DIR_UP;
            break;
        case 's':
        case 'S':
            protocol_dir = DIR_DOWN;
            break;
        case 'a':
        case 'A':
            protocol_dir = DIR_LEFT;
            break;
        case 'd':
        case 'D':
            protocol_dir = DIR_RIGHT;
            break;
        default:
            return 0;
    }

    snprintf(command, sizeof(command), "%s %s\n", CMD_MOVE, protocol_dir);
    return send_all(sock, command, strlen(command));
}

/*
 * Handles one key read while the client is in movement mode.
 *
 * Input:
 * - sock: connected socket descriptor.
 * - state: current client state.
 * - key: character read from standard input.
 *
 * Output:
 * - Returns 0 on success, 1 when the client should quit, and -1 on error.
 * - May update mode and map overlay state.
 *
 * Behavior:
 * - Sends movement commands immediately.
 * - TAB switches to command mode.
 * - G toggles global overlay and requests GLOBAL.
 * - L disables the global overlay and requests LOCAL.
 */
static int handle_movement_input(int sock, ClientState *state, char key) {
    if (key == '\t') {
        state->mode = COMMAND;
        state->input_pos = 0;
        state->input_buffer[0] = '\0';
        state_set_status(state, "Command mode enabled.");
        draw_screen(state);
        return 0;
    }

    if (key == 'q' || key == 'Q') {
        send_all(sock, "QUIT\n", strlen("QUIT\n"));
        return 1;
    }

    if (key == 'g' || key == 'G') {
        state->show_global = !state->show_global;
        send_all(sock, "GLOBAL\n", strlen("GLOBAL\n"));
        draw_screen(state);
        return 0;
    }

    if (key == 'l' || key == 'L') {
        state->show_global = 0;
        send_all(sock, "LOCAL\n", strlen("LOCAL\n"));
        draw_screen(state);
        return 0;
    }

    if (key == 'w' || key == 'W' ||
        key == 'a' || key == 'A' ||
        key == 's' || key == 'S' ||
        key == 'd' || key == 'D') {
        return send_movement_key(sock, key);
    }

    return 0;
}

/*
 * Handles one key read while the client is in command mode.
 *
 * Input:
 * - sock: connected socket descriptor.
 * - state: current client state.
 * - key: character read from standard input.
 *
 * Output:
 * - Returns 0 on success, 1 when the client should quit, and -1 on error.
 * - May update the command input buffer.
 *
 * Behavior:
 * - Maintains a manual input buffer because raw mode disables fgets().
 * - ENTER sends the current command.
 * - TAB switches back to movement mode.
 */
static int handle_command_input(int sock, ClientState *state, char key) {
    if (key == '\t') {
        state->mode = MOVEMENT;
        state_set_status(state, "Movement mode enabled.");
        draw_screen(state);
        return 0;
    }

    if (key == '\r' || key == '\n') {
        char command[MAX_MSG];

        snprintf(command, sizeof(command), "%s", state->input_buffer);
        state->input_pos = 0;
        state->input_buffer[0] = '\0';

        if (strcmp(command, "quit") == 0 || strcmp(command, "/quit") == 0) {
            send_user_command(sock, state, command);
            return 1;
        }

        if (send_user_command(sock, state, command) < 0) {
            return -1;
        }

        draw_screen(state);
        return 0;
    }

    if (key == 127 || key == '\b') {
        if (state->input_pos > 0) {
            state->input_pos--;
            state->input_buffer[state->input_pos] = '\0';
        }

        draw_screen(state);
        return 0;
    }

    if ((unsigned char)key >= 32 && state->input_pos < MAX_MSG - 1) {
        state->input_buffer[state->input_pos++] = key;
        state->input_buffer[state->input_pos] = '\0';
        draw_screen(state);
    }

    return 0;
}

/*
 * Client process entry point.
 *
 * Input:
 * - argc: number of command-line arguments.
 * - argv: server host and server port.
 *
 * Output:
 * - Returns 0 on normal termination and 1 on startup error.
 *
 * Behavior:
 * - Connects to the TCP server.
 * - Uses select() to monitor standard input and the server socket.
 * - Uses termios raw mode only on the client side.
 * - Restores the terminal before exiting.
 */
int main(int argc, char *argv[]) {
    int sock;
    fd_set readfds;
    char first_line[MAX_MSG];
    ClientState state;
    int raw_enabled;

    if (argc != 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }

    init_client_state(&state);
    state.mode = COMMAND;

    sock = connect_to_server(argv[1], argv[2]);
    if (sock < 0) {
        printf("Unable to connect to server %s:%s\n", argv[1], argv[2]);
        return 1;
    }

    raw_enabled = (terminal_set_raw() == 0);
    if (!raw_enabled) {
        state.mode = COMMAND;
        state_set_status(&state, "Raw terminal mode unavailable. Command mode only.");
    }

    atexit(terminal_restore);
    draw_screen(&state);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        if (select(sock + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char key;
            ssize_t n = read(STDIN_FILENO, &key, 1);
            int input_result;

            if (n <= 0) {
                break;
            }

            if (state.mode == MOVEMENT && raw_enabled) {
                input_result = handle_movement_input(sock, &state, key);
            } else {
                input_result = handle_command_input(sock, &state, key);
            }

            if (input_result > 0) {
                break;
            }

            if (input_result < 0) {
                state_set_status(&state, "Send error.");
                draw_screen(&state);
                break;
            }
        }

        if (FD_ISSET(sock, &readfds)) {
            if (recv_line(sock, first_line, sizeof(first_line)) < 0) {
                state_set_status(&state, "Disconnected from server.");
                draw_screen(&state);
                break;
            }

            if (handle_server_message(sock, first_line, &state) < 0) {
                state_set_status(&state, "Protocol error or disconnected server.");
                draw_screen(&state);
                break;
            }
        }
    }

    terminal_restore();
    close(sock);
    printf("\nDisconnected.\n");
    return 0;
}
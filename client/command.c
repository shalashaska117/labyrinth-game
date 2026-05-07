#include "command.h"

#include "../common/protocol.h"

#include <stdio.h>
#include <string.h>

/*
 * Removes a leading slash from command-mode input.
 *
 * Input:
 * - input: user command string.
 *
 * Output:
 * - Returns input without the first slash when present.
 * - Returns NULL if input is NULL.
 *
 * Behavior:
 * - Supports both commands such as ready and slash commands such as /ready.
 * - Does not modify the original string.
 */
static const char *skip_optional_slash(const char *input) {
    if (input != NULL && input[0] == '/') {
        return input + 1;
    }

    return input;
}

/*
 * Converts a user-friendly command into a protocol command.
 *
 * Input:
 * - input: command typed by the user.
 * - output: destination buffer for the protocol command.
 * - size: size of the destination buffer.
 *
 * Output:
 * - Returns 1 if the command must be sent to the server.
 * - Returns 0 if the command was handled locally.
 * - Returns -1 if the command is invalid.
 *
 * Behavior:
 * - Maps lowercase commands to uppercase protocol constants.
 * - Supports movement shortcuts and slash commands.
 * - Keeps RANK as the primary scoreboard protocol command.
 * - Treats help as a local command without printing directly to the terminal.
 */
int build_protocol_command(const char *input, char *output, size_t size) {
    char nickname[MAX_NICK];
    char password[MAX_PASSWORD];
    const char *cmd;

    if (input == NULL || output == NULL || size == 0) {
        return -1;
    }

    cmd = skip_optional_slash(input);

    if (cmd == NULL) {
        return -1;
    }

    if (strcmp(cmd, "w") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_UP);
        return 1;
    }

    if (strcmp(cmd, "s") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_DOWN);
        return 1;
    }

    if (strcmp(cmd, "a") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_LEFT);
        return 1;
    }

    if (strcmp(cmd, "d") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_RIGHT);
        return 1;
    }

    if (strcmp(cmd, "local") == 0) {
        snprintf(output, size, "%s\n", CMD_LOCAL);
        return 1;
    }

    if (strcmp(cmd, "global") == 0) {
        snprintf(output, size, "%s\n", CMD_GLOBAL);
        return 1;
    }

    if (strcmp(cmd, "users") == 0 || strcmp(cmd, "list") == 0) {
        snprintf(output, size, "%s\n", CMD_LIST);
        return 1;
    }

    if (strcmp(cmd, "ready") == 0) {
        snprintf(output, size, "%s\n", CMD_READY);
        return 1;
    }

    if (strcmp(cmd, "start") == 0) {
        snprintf(output, size, "%s\n", CMD_START);
        return 1;
    }

    if (strcmp(cmd, "reset") == 0) {
        snprintf(output, size, "%s\n", CMD_RESET);
        return 1;
    }

    if (strcmp(cmd, "rank") == 0 || strcmp(cmd, "scoreboard") == 0) {
        snprintf(output, size, "%s\n", CMD_RANK);
        return 1;
    }

    if (strcmp(cmd, "quit") == 0) {
        snprintf(output, size, "%s\n", CMD_QUIT);
        return 1;
    }

    if (strcmp(cmd, "help") == 0) {
        return 0;
    }

    if (sscanf(cmd, "register %31s %63s", nickname, password) == 2) {
        snprintf(output, size, "%s %s %s\n", CMD_REGISTER, nickname, password);
        return 1;
    }

    if (sscanf(cmd, "login %31s %63s", nickname, password) == 2) {
        snprintf(output, size, "%s %s %s\n", CMD_LOGIN, nickname, password);
        return 1;
    }

    return -1;
}
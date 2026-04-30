#include "command.h"
#include "ui.h"

#include "../common/protocol.h"

#include <stdio.h>
#include <string.h>

/*
 * Converts a user command into a protocol command.
 *
 * The parser keeps the user interface simple. The player can type short
 * commands like "w", while the server receives explicit protocol messages
 * such as "MOVE UP".
 */
int build_protocol_command(const char *input, char *output, size_t size) {
    char nickname[MAX_NICK];
    char password[MAX_PASSWORD];

    if (input == NULL || output == NULL || size == 0) {
        return -1;
    }

    if (strcmp(input, "w") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_UP);
        return 1;
    }

    if (strcmp(input, "s") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_DOWN);
        return 1;
    }

    if (strcmp(input, "a") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_LEFT);
        return 1;
    }

    if (strcmp(input, "d") == 0) {
        snprintf(output, size, "%s %s\n", CMD_MOVE, DIR_RIGHT);
        return 1;
    }

    if (strcmp(input, "local") == 0) {
        snprintf(output, size, "%s\n", CMD_LOCAL);
        return 1;
    }

    if (strcmp(input, "global") == 0) {
        snprintf(output, size, "%s\n", CMD_GLOBAL);
        return 1;
    }

    if (strcmp(input, "users") == 0) {
        snprintf(output, size, "%s\n", CMD_LIST);
        return 1;
    }

    if (strcmp(input, "quit") == 0) {
        snprintf(output, size, "%s\n", CMD_QUIT);
        return 1;
    }

    if (strcmp(input, "help") == 0) {
        print_help();
        return 0;
    }

    if (sscanf(input, "register %31s %63s", nickname, password) == 2) {
        snprintf(output, size, "%s %s %s\n", CMD_REGISTER, nickname, password);
        return 1;
    }

    if (sscanf(input, "login %31s %63s", nickname, password) == 2) {
        snprintf(output, size, "%s %s %s\n", CMD_LOGIN, nickname, password);
        return 1;
    }

    return -1;
}

#include "response.h"
#include "ui.h"

#include "../common/protocol.h"
#include "../common/constants.h"

#include <stdio.h>
#include <string.h>

/*
 * Reads and prints a MAP message from the server.
 *
 * The function receives the remaining rows of a multi-line MAP response,
 * verifies the END marker, and forwards the parsed matrix to the UI module.
 */
static int handle_map_message(int sock, const char *header) {
    char map_type[32];
    int rows;
    int cols;
    char line[MAX_MSG];
    char map[MAP_WIDTH * MAP_HEIGHT];
    int r;
    int c;

    if (sscanf(header, "%*s %31s %d %d", map_type, &rows, &cols) != 3) {
        printf("Invalid MAP header received from server.\n");
        return -1;
    }

    if (rows <= 0 || cols <= 0 || rows > MAP_HEIGHT || cols > MAP_WIDTH) {
        printf("Invalid MAP size received from server.\n");
        return -1;
    }

    for (r = 0; r < rows; r++) {
        if (recv_line(sock, line, sizeof(line)) < 0) {
            return -1;
        }

        for (c = 0; c < cols; c++) {
            if (c < (int)strlen(line)) {
                map[r * cols + c] = line[c];
            } else {
                map[r * cols + c] = CELL_HIDDEN;
            }
        }
    }

    if (recv_line(sock, line, sizeof(line)) < 0) {
        return -1;
    }

    if (strcmp(line, RESP_END) != 0) {
        printf("Invalid MAP termination received from server.\n");
        return -1;
    }

    if (strcmp(map_type, MAP_LOCAL) == 0) {
        print_map("Local Map", map, rows, cols);
    } else if (strcmp(map_type, MAP_GLOBAL) == 0) {
        print_map("Global Masked Map", map, rows, cols);
    } else {
        print_map("Map", map, rows, cols);
    }

    return 0;
}

/*
 * Reads and prints a USERS message from the server.
 */
static int handle_users_message(int sock, const char *header) {
    int count;
    int i;
    char line[MAX_MSG];
    char users[MAX_USERS_DISPLAY][MAX_NICK];

    if (sscanf(header, "%*s %d", &count) != 1) {
        printf("Invalid USERS header received from server.\n");
        return -1;
    }

    if (count < 0 || count > MAX_USERS_DISPLAY) {
        printf("Invalid USERS count received from server.\n");
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (recv_line(sock, line, sizeof(line)) < 0) {
            return -1;
        }

        strncpy(users[i], line, MAX_NICK - 1);
        users[i][MAX_NICK - 1] = '\0';
    }

    if (recv_line(sock, line, sizeof(line)) < 0) {
        return -1;
    }

    if (strcmp(line, RESP_END) != 0) {
        printf("Invalid USERS termination received from server.\n");
        return -1;
    }

    print_users(users, count);
    return 0;
}

/*
 * Handles a server message and updates client state when needed.
 */
int handle_server_message(int sock, const char *first_line, ClientState *state) {
    if (first_line == NULL) {
        return -1;
    }

    if (strncmp(first_line, RESP_OK, strlen(RESP_OK)) == 0) {
        printf("[OK]%s\n", first_line + strlen(RESP_OK));

        if (state != NULL && state->pending_auth) {
            confirm_auth(state);
            printf("Logged in as %s\n", state->nickname);
        }

        return 0;
    }

    if (strncmp(first_line, RESP_ERR, strlen(RESP_ERR)) == 0) {
        printf("[ERROR]%s\n", first_line + strlen(RESP_ERR));

        if (state != NULL && state->pending_auth) {
            clear_pending_auth(state);
        }

        return 0;
    }

    if (strncmp(first_line, RESP_MAP, strlen(RESP_MAP)) == 0) {
        return handle_map_message(sock, first_line);
    }

    if (strncmp(first_line, RESP_USERS, strlen(RESP_USERS)) == 0) {
        return handle_users_message(sock, first_line);
    }

    printf("[SERVER] %s\n", first_line);
    return 0;
}

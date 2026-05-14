#include "response.h"
#include "ui.h"

#include "../common/constants.h"
#include "../common/protocol.h"

#include <stdio.h>
#include <string.h>

/*
 * Stores a parsed map into the client state.
 *
 * Input:
 * - state: client state to update.
 * - map_type: LOCAL or GLOBAL map type.
 * - map: flat map buffer.
 * - rows: number of map rows.
 * - cols: number of map columns.
 *
 * Output:
 * - Updates the matching map buffer and dimensions.
 *
 * Behavior:
 * - Copies only within the fixed state buffers.
 * - Ignores unknown map types.
 */
static void store_map(ClientState *state, const char *map_type, const char *map, int rows, int cols) {
    size_t total = (size_t)rows * (size_t)cols;

    if (state == NULL || map_type == NULL || map == NULL) {
        return;
    }

    if (strcmp(map_type, MAP_LOCAL) == 0) {
        size_t max_total = sizeof(state->local_map);

        if (total > max_total) {
            total = max_total;
        }

        memcpy(state->local_map, map, total);
        state->local_rows = rows;
        state->local_cols = cols;
    } else if (strcmp(map_type, MAP_GLOBAL) == 0) {
        size_t max_total = sizeof(state->global_map);

        if (total > max_total) {
            total = max_total;
        }

        memcpy(state->global_map, map, total);
        state->global_rows = rows;
        state->global_cols = cols;
    }
}

/*
 * Clears per-session visual data after returning to the lobby.
 *
 * Input:
 * - state: client state to reset.
 *
 * Output:
 * - Clears maps, ranking and overlay flags in the client state.
 *
 * Behavior:
 * - Keeps authentication, nickname and owner information unchanged.
 * - Resets only visual/gameplay data belonging to the completed session.
 */
static void clear_session_display(ClientState *state) {
    if (state == NULL) {
        return;
    }

    state->show_global = 0;
    state->rank_count = 0;

    memset(state->rank_lines, 0, sizeof(state->rank_lines));
    memset(state->local_map, CELL_HIDDEN, sizeof(state->local_map));
    memset(state->global_map, CELL_HIDDEN, sizeof(state->global_map));

    state->local_rows = VIEW_RADIUS * 2 + 1;
    state->local_cols = VIEW_RADIUS * 2 + 1;
    state->global_rows = MAP_HEIGHT;
    state->global_cols = MAP_WIDTH;
}

/*
 * Reads and stores a MAP message from the server.
 *
 * Input:
 * - sock: socket connected to the server.
 * - header: first MAP line already received.
 * - state: client state to update.
 *
 * Output:
 * - Returns 0 on success and -1 on protocol error.
 * - Updates the local or global map in state.
 *
 * Behavior:
 * - Reads exactly the announced number of rows.
 * - Requires END after the map body.
 */
static int handle_map_message(int sock, const char *header, ClientState *state) {
    char map_type[32];
    int rows;
    int cols;
    char line[MAX_MSG];
    char map[MAP_WIDTH * MAP_HEIGHT];

    if (sscanf(header, "%*s %31s %d %d", map_type, &rows, &cols) != 3) {
        state_set_status(state, "Invalid MAP header received from server.");
        return -1;
    }

    if (rows <= 0 || cols <= 0 || rows > MAP_HEIGHT || cols > MAP_WIDTH) {
        state_set_status(state, "Invalid MAP size received from server.");
        return -1;
    }

    for (int r = 0; r < rows; r++) {
        if (recv_line(sock, line, sizeof(line)) < 0) {
            return -1;
        }

        for (int c = 0; c < cols; c++) {
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
        state_set_status(state, "Invalid MAP termination received from server.");
        return -1;
    }

    store_map(state, map_type, map, rows, cols);
    state_set_status(state, strcmp(map_type, MAP_GLOBAL) == 0 ? "Global map updated." : "Local map updated.");
    draw_screen(state);
    return 0;
}

/*
 * Reads and stores a USERS message from the server.
 *
 * Input:
 * - sock: socket connected to the server.
 * - header: first USERS line already received.
 * - state: client state to update.
 *
 * Output:
 * - Returns 0 on success and -1 on protocol error.
 * - Updates player list fields in state.
 *
 * Behavior:
 * - Reads count user lines and the final END marker.
 * - Detects owner marker in the current user's line when possible.
 */
static int handle_users_message(int sock, const char *header, ClientState *state) {
    int count;
    char line[MAX_MSG];

    if (sscanf(header, "%*s %d", &count) != 1) {
        state_set_status(state, "Invalid USERS header received from server.");
        return -1;
    }

    if (count < 0 || count > MAX_USERS_DISPLAY) {
        state_set_status(state, "Invalid USERS count received from server.");
        return -1;
    }

    if (state != NULL) {
        state->player_count = count;
        state->is_owner = 0;
    }

    for (int i = 0; i < count; i++) {
        if (recv_line(sock, line, sizeof(line)) < 0) {
            return -1;
        }

        if (state != NULL) {
            snprintf(state->player_list[i], sizeof(state->player_list[i]), "%s", line);

            if (state->nickname[0] != '\0' &&
                strstr(line, state->nickname) != NULL &&
                strstr(line, "[owner]") != NULL) {
                state->is_owner = 1;
            }
        }
    }

    if (recv_line(sock, line, sizeof(line)) < 0) {
        return -1;
    }

    if (strcmp(line, RESP_END) != 0) {
        state_set_status(state, "Invalid USERS termination received from server.");
        return -1;
    }

    state_set_status(state, "User list updated.");
    draw_screen(state);
    return 0;
}

/*
 * Reads and stores a RANK message from the server.
 *
 * Input:
 * - sock: socket connected to the server.
 * - header: first RANK line already received.
 * - state: client state to update.
 *
 * Output:
 * - Returns 0 on success and -1 on protocol error.
 * - Updates ranking lines in state.
 *
 * Behavior:
 * - Reads exactly the announced number of ranking entries.
 * - Requires END after the ranking body.
 */
static int handle_rank_message(int sock, const char *header, ClientState *state) {
    int count;
    char line[MAX_MSG];

    if (sscanf(header, "%*s %d", &count) != 1) {
        state_set_status(state, "Invalid RANK header received from server.");
        return -1;
    }

    if (count < 0 || count > MAX_USERS_DISPLAY) {
        state_set_status(state, "Invalid RANK count received from server.");
        return -1;
    }

    if (state != NULL) {
        state->rank_count = count;
    }

    for (int i = 0; i < count; i++) {
        if (recv_line(sock, line, sizeof(line)) < 0) {
            return -1;
        }

        if (state != NULL) {
            snprintf(state->rank_lines[i], sizeof(state->rank_lines[i]), "%s", line);
        }
    }

    if (recv_line(sock, line, sizeof(line)) < 0) {
        return -1;
    }

    if (strcmp(line, RESP_END) != 0) {
        state_set_status(state, "Invalid RANK termination received from server.");
        return -1;
    }

    state_set_status(state, "Ranking updated.");
    draw_screen(state);
    return 0;
}

/*
 * Handles one complete server response.
 *
 * Input:
 * - sock: socket connected to the server.
 * - first_line: first line already received from the server.
 * - state: client state to update.
 *
 * Output:
 * - Returns 0 on success and -1 on protocol or connection error.
 * - Updates maps, users, rank, session and status fields when needed.
 *
 * Behavior:
 * - Dispatches multi-line responses to specialized parsers.
 * - Updates the persistent terminal screen after relevant changes.
 */
int handle_server_message(int sock, const char *first_line, ClientState *state) {
    if (first_line == NULL) {
        return -1;
    }

    if (strncmp(first_line, RESP_OK, strlen(RESP_OK)) == 0) {
        state_set_status(state, first_line);

        if (state != NULL && strstr(first_line, "exit found") != NULL) {
            state->exit_reached = 1;
            state->show_global = 1;
        }

        if (state != NULL && strstr(first_line, "owner") != NULL) {
            state->is_owner = 1;
        }

        if (state != NULL && state->pending_auth) {
            confirm_auth(state);
        }

        draw_screen(state);
        return 0;
    }

    if (strncmp(first_line, RESP_ERR, strlen(RESP_ERR)) == 0) {
        state_set_status(state, first_line);

        if (state != NULL && state->pending_auth) {
            clear_pending_auth(state);
        }

        draw_screen(state);
        return 0;
    }

    if (strncmp(first_line, "TIME", 4) == 0) {
        int sec = 0;
        if (sscanf(first_line, "TIME %d", &sec) == 1 && state != NULL) {
            state->time_remaining = sec;
        }

        draw_screen(state);
        return 0;
    }

    if (strcmp(first_line, "SESSION STARTED") == 0) {
        if (state != NULL) {
            state->session_state = CLIENT_PLAYING;
            state->mode = MOVEMENT;
            state->show_global = 0;
            state->rank_count = 0;
        }

        state_set_status(state, "SESSION STARTED");
        draw_screen(state);
        return 0;
    }

    if (strcmp(first_line, "SESSION ENDED") == 0) {
        if (state != NULL) {
            state->session_state = CLIENT_FINISHED;
            state->mode = COMMAND;
            state->show_global = 0;
        }

        state_set_status(state, "SESSION ENDED");
        send_all(sock, "RANK\n", strlen("RANK\n"));
        draw_screen(state);
        return 0;
    }

    if (strcmp(first_line, "SESSION LOBBY") == 0) {
        if (state != NULL) {
            state->session_state = CLIENT_LOBBY;
            state->mode = COMMAND;
            clear_session_display(state);
        }

        state_set_status(state, "Session reset. Back to lobby.");
        send_all(sock, "LIST\n", strlen("LIST\n"));
        draw_screen(state);
        return 0;
    }

    if (strncmp(first_line, RESP_MAP, strlen(RESP_MAP)) == 0) {
        return handle_map_message(sock, first_line, state);
    }

    if (strncmp(first_line, RESP_USERS, strlen(RESP_USERS)) == 0) {
        return handle_users_message(sock, first_line, state);
    }

    if (strncmp(first_line, RESP_RANK, strlen(RESP_RANK)) == 0) {
        return handle_rank_message(sock, first_line, state);
    }

    state_set_status(state, first_line);
    draw_screen(state);
    return 0;
}
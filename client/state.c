#include "state.h"

#include <stdio.h>
#include <string.h>

/*
 * Initializes the client state.
 *
 * Input:
 * - state: client state structure to initialize.
 *
 * Output:
 * - The structure is reset to its default values.
 *
 * Behavior:
 * - Clears authentication fields.
 * - Starts the client in lobby state.
 * - Starts the input system in command mode.
 */
void init_client_state(ClientState *state) {
    int i;

    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(ClientState));

    state->mode = COMMAND;
    state->session_state = CLIENT_LOBBY;
    state->show_global = 0;

    for (i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) {
        state->global_map[i] = CELL_HIDDEN;
    }

    for (i = 0; i < (VIEW_RADIUS * 2 + 1) * (VIEW_RADIUS * 2 + 1); i++) {
        state->local_map[i] = CELL_HIDDEN;
    }

    snprintf(state->status_message,
             sizeof(state->status_message),
             "Connected. Register or login, then use ready/start.");
}

/*
 * Stores the nickname used by a pending authentication command.
 *
 * Input:
 * - state: client state to update.
 * - nickname: nickname used in the login or register command.
 *
 * Output:
 * - Updates pending_nickname and pending_auth.
 *
 * Behavior:
 * - Ignores null input.
 * - Copies the nickname with snprintf to preserve bounds.
 */
void set_pending_auth(ClientState *state, const char *nickname) {
    if (state == NULL || nickname == NULL) {
        return;
    }

    snprintf(state->pending_nickname, sizeof(state->pending_nickname), "%s", nickname);
    state->pending_auth = 1;
}

/*
 * Confirms a successful authentication response.
 *
 * Input:
 * - state: client state to update.
 *
 * Output:
 * - Updates logged_in, nickname and pending authentication fields.
 *
 * Behavior:
 * - Only changes the visible nickname when an auth command was pending.
 * - Clears the pending nickname after confirmation.
 */
void confirm_auth(ClientState *state) {
    if (state == NULL) {
        return;
    }

    if (state->pending_auth) {
        snprintf(state->nickname, sizeof(state->nickname), "%s", state->pending_nickname);
        state->logged_in = 1;
        state->pending_auth = 0;
        memset(state->pending_nickname, 0, sizeof(state->pending_nickname));
    }
}

/*
 * Clears a pending authentication request.
 *
 * Input:
 * - state: client state to update.
 *
 * Output:
 * - Resets pending_auth and pending_nickname.
 *
 * Behavior:
 * - Used after authentication errors.
 */
void clear_pending_auth(ClientState *state) {
    if (state == NULL) {
        return;
    }

    state->pending_auth = 0;
    memset(state->pending_nickname, 0, sizeof(state->pending_nickname));
}

/*
 * Updates the status message shown in the terminal UI.
 *
 * Input:
 * - state: client state to update.
 * - message: message to display.
 *
 * Output:
 * - Writes the new status message into the state.
 *
 * Behavior:
 * - Ignores null inputs.
 * - Uses snprintf to keep the message bounded.
 */
void state_set_status(ClientState *state, const char *message) {
    if (state == NULL || message == NULL) {
        return;
    }

    snprintf(state->status_message, sizeof(state->status_message), "%s", message);
}

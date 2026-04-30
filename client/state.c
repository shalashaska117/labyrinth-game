#include "state.h"

#include <string.h>
#include <stdio.h>

/*
 * Initializes the client state.
 */
void init_client_state(ClientState *state) {
    if (state == NULL) {
        return;
    }

    memset(state->nickname, 0, sizeof(state->nickname));
    memset(state->pending_nickname, 0, sizeof(state->pending_nickname));

    state->logged_in = 0;
    state->pending_auth = 0;
}

/*
 * Stores the nickname used in a pending authentication request.
 */
void set_pending_auth(ClientState *state, const char *nickname) {
    if (state == NULL || nickname == NULL) {
        return;
    }

    snprintf(state->pending_nickname, sizeof(state->pending_nickname), "%s", nickname);
    state->pending_auth = 1;
}

/*
 * Confirms authentication after a positive server response.
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
 * Clears pending authentication after an error response.
 */
void clear_pending_auth(ClientState *state) {
    if (state == NULL) {
        return;
    }

    state->pending_auth = 0;
    memset(state->pending_nickname, 0, sizeof(state->pending_nickname));
}

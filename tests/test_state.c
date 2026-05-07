#include <stdio.h>
#include <string.h>

#include "../client/state.h"

/*
 * Checks the client state initialization and authentication flow.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if all checks pass and 1 otherwise.
 *
 * Behavior:
 * - Verifies initial lobby state.
 * - Verifies that the client starts in command mode.
 * - Verifies pending and confirmed authentication transitions.
 */
int main(void) {
    ClientState state;

    init_client_state(&state);

    if (state.logged_in != 0 || state.pending_auth != 0) {
        printf("FAILED: initial authentication state is invalid\n");
        return 1;
    }

    if (state.session_state != CLIENT_LOBBY || state.mode != COMMAND) {
        printf("FAILED: initial UI/session state is invalid\n");
        return 1;
    }

    if (state.show_global != 0) {
        printf("FAILED: initial global overlay state is invalid\n");
        return 1;
    }

    if (state.rank_count != 0 || state.player_count != 0) {
        printf("FAILED: initial list/rank state is invalid\n");
        return 1;
    }

    set_pending_auth(&state, "pietro");

    if (state.pending_auth != 1) {
        printf("FAILED: pending auth not set\n");
        return 1;
    }

    confirm_auth(&state);

    if (state.logged_in != 1 || strcmp(state.nickname, "pietro") != 0) {
        printf("FAILED: authentication confirmation failed\n");
        return 1;
    }

    clear_pending_auth(&state);

    if (state.pending_auth != 0) {
        printf("FAILED: pending auth not cleared\n");
        return 1;
    }

    state_set_status(&state, "status ok");

    if (strcmp(state.status_message, "status ok") != 0) {
        printf("FAILED: status message not updated\n");
        return 1;
    }

    printf("PASSED: client state test\n");
    return 0;
}
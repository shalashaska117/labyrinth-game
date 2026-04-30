#include <stdio.h>
#include <string.h>

#include "../client/state.h"

/*
 * Checks the client state initialization and authentication flow.
 */
int main(void) {
    ClientState state;

    init_client_state(&state);

    if (state.logged_in != 0 || state.pending_auth != 0) {
        printf("FAILED: initial state is invalid\n");
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

    printf("PASSED: client state test\n");
    return 0;
}

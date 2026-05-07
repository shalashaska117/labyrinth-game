#include <stdio.h>
#include <string.h>

#include "../client/state.h"

/*
 * Reports a failed extended state test.
 *
 * Input:
 * - message: failure explanation.
 *
 * Output:
 * - Returns 0 to the caller.
 *
 * Behavior:
 * - Keeps diagnostics consistent with the existing tests.
 */
static int fail(const char *message) {
    printf("FAILED: %s\n", message);
    return 0;
}

/*
 * Tests null-safe client state functions.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Exercises guard clauses that are not covered by the basic state test.
 * - Verifies that no function crashes when optional inputs are missing.
 */
static int test_null_guards(void) {
    ClientState state;

    init_client_state(NULL);

    init_client_state(&state);

    set_pending_auth(NULL, "pietro");
    set_pending_auth(&state, NULL);
    confirm_auth(NULL);
    clear_pending_auth(NULL);
    state_set_status(NULL, "ignored");
    state_set_status(&state, NULL);

    if (state.logged_in != 0 || state.pending_auth != 0) {
        return fail("null guard calls changed authentication state");
    }

    printf("PASSED: state null guards\n");
    return 1;
}

/*
 * Tests pending authentication overwrite and failed confirmation behavior.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Verifies pending nickname overwrite.
 * - Verifies that confirm_auth() does not authenticate without a pending command.
 */
static int test_pending_auth_edges(void) {
    ClientState state;

    init_client_state(&state);

    confirm_auth(&state);
    if (state.logged_in != 0 || state.nickname[0] != '\0') {
        return fail("confirm_auth authenticated without pending auth");
    }

    set_pending_auth(&state, "first");
    set_pending_auth(&state, "second");
    confirm_auth(&state);

    if (!state.logged_in || strcmp(state.nickname, "second") != 0) {
        return fail("pending authentication overwrite failed");
    }

    clear_pending_auth(&state);
    if (state.pending_auth != 0 || state.pending_nickname[0] != '\0') {
        return fail("pending authentication was not cleared");
    }

    printf("PASSED: pending authentication edges\n");
    return 1;
}

/*
 * Extended state test entry point.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if all checks pass and 1 otherwise.
 *
 * Behavior:
 * - Complements tests/test_state.c without replacing it.
 */
int main(void) {
    int passed = 0;
    int total = 0;

    total++; passed += test_null_guards();
    total++; passed += test_pending_auth_edges();

    printf("\nExtended state tests: %d/%d passed\n", passed, total);
    return passed == total ? 0 : 1;
}

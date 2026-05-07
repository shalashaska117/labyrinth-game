#include <stdio.h>
#include <string.h>

#include "../client/command.h"
#include "../common/protocol.h"

/*
 * Checks one command translation.
 *
 * Input:
 * - input: command string.
 * - expected_result: expected return value.
 * - expected_output: expected protocol line, or NULL when output is not checked.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Mirrors the style of tests/test_command_parser.c.
 */
static int check_command(const char *input, int expected_result, const char *expected_output) {
    char output[MAX_MSG];
    int result;

    memset(output, 0, sizeof(output));
    result = build_protocol_command(input, output, sizeof(output));

    if (result != expected_result) {
        printf("FAILED: input='%s' expected result %d, got %d\n",
               input != NULL ? input : "(null)",
               expected_result,
               result);
        return 0;
    }

    if (expected_output != NULL && strcmp(output, expected_output) != 0) {
        printf("FAILED: input='%s' expected output '%s', got '%s'\n",
               input != NULL ? input : "(null)",
               expected_output,
               output);
        return 0;
    }

    printf("PASSED: %s\n", input != NULL ? input : "(null)");
    return 1;
}

/*
 * Extended command parser test entry point.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if all checks pass and 1 otherwise.
 *
 * Behavior:
 * - Covers slash aliases and parser guard clauses not covered by the base test.
 */
int main(void) {
    char output[MAX_MSG];
    int passed = 0;
    int total = 0;

    total++; passed += check_command("/w", 1, "MOVE UP\n");
    total++; passed += check_command("/s", 1, "MOVE DOWN\n");
    total++; passed += check_command("/a", 1, "MOVE LEFT\n");
    total++; passed += check_command("/d", 1, "MOVE RIGHT\n");
    total++; passed += check_command("/local", 1, "LOCAL\n");
    total++; passed += check_command("/global", 1, "GLOBAL\n");
    total++; passed += check_command("/users", 1, "LIST\n");
    total++; passed += check_command("/list", 1, "LIST\n");
    total++; passed += check_command("/ready", 1, "READY\n");
    total++; passed += check_command("/start", 1, "START\n");
    total++; passed += check_command("/scoreboard", 1, "RANK\n");
    total++; passed += check_command("/quit", 1, "QUIT\n");
    total++; passed += check_command("/help", 0, NULL);
    total++; passed += check_command("register missing_password", -1, NULL);
    total++; passed += check_command("login missing_password", -1, NULL);
    total++; passed += check_command(NULL, -1, NULL);

    memset(output, 0, sizeof(output));
    total++;
    if (build_protocol_command("ready", output, 0) == -1) {
        printf("PASSED: zero-sized output buffer\n");
        passed++;
    } else {
        printf("FAILED: zero-sized output buffer\n");
    }

    total++;
    if (build_protocol_command("ready", NULL, sizeof(output)) == -1) {
        printf("PASSED: null output buffer\n");
        passed++;
    } else {
        printf("FAILED: null output buffer\n");
    }

    printf("\nExtended command parser tests: %d/%d passed\n", passed, total);
    return passed == total ? 0 : 1;
}

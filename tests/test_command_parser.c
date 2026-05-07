#include <stdio.h>
#include <string.h>

#include "../client/command.h"
#include "../common/protocol.h"

/*
 * Checks whether a command is translated correctly.
 *
 * Input:
 * - input: user command to parse.
 * - expected_result: expected parser result.
 * - expected_output: expected protocol string, or NULL if not checked.
 *
 * Output:
 * - Returns 1 if the check passes and 0 otherwise.
 *
 * Behavior:
 * - Prints one diagnostic line for every failed condition.
 */
static int check_command(const char *input, int expected_result, const char *expected_output) {
    char output[MAX_MSG];
    int result;

    memset(output, 0, sizeof(output));
    result = build_protocol_command(input, output, sizeof(output));

    if (result != expected_result) {
        printf("FAILED: input='%s' expected result %d, got %d\n", input, expected_result, result);
        return 0;
    }

    if (expected_output != NULL && strcmp(output, expected_output) != 0) {
        printf("FAILED: input='%s' expected output '%s', got '%s'\n", input, expected_output, output);
        return 0;
    }

    printf("PASSED: %s\n", input);
    return 1;
}

/*
 * Unit test entry point for the command parser.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if all tests pass and 1 otherwise.
 *
 * Behavior:
 * - Verifies movement, authentication, lobby, reset and ranking commands.
 */
int main(void) {
    int passed = 0;
    int total = 0;

    total++; passed += check_command("w", 1, "MOVE UP\n");
    total++; passed += check_command("s", 1, "MOVE DOWN\n");
    total++; passed += check_command("a", 1, "MOVE LEFT\n");
    total++; passed += check_command("d", 1, "MOVE RIGHT\n");
    total++; passed += check_command("local", 1, "LOCAL\n");
    total++; passed += check_command("global", 1, "GLOBAL\n");
    total++; passed += check_command("users", 1, "LIST\n");
    total++; passed += check_command("ready", 1, "READY\n");
    total++; passed += check_command("start", 1, "START\n");
    total++; passed += check_command("reset", 1, "RESET\n");
    total++; passed += check_command("/reset", 1, "RESET\n");
    total++; passed += check_command("rank", 1, "RANK\n");
    total++; passed += check_command("/rank", 1, "RANK\n");
    total++; passed += check_command("scoreboard", 1, "RANK\n");
    total++; passed += check_command("quit", 1, "QUIT\n");
    total++; passed += check_command("register pietro secret", 1, "REGISTER pietro secret\n");
    total++; passed += check_command("login pietro secret", 1, "LOGIN pietro secret\n");
    total++; passed += check_command("invalid", -1, NULL);

    printf("\nCommand parser tests: %d/%d passed\n", passed, total);
    return passed == total ? 0 : 1;
}
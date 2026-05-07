#include "ui.h"

#include "../common/constants.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define BORDER_TOP_LEFT "┌"
#define BORDER_TOP_RIGHT "┐"
#define BORDER_BOTTOM_LEFT "└"
#define BORDER_BOTTOM_RIGHT "┘"
#define BORDER_HORIZONTAL "─"
#define BORDER_VERTICAL "│"

#define UI_CONTENT_WIDTH 88
#define UI_BORDER_WIDTH 90
#define MAP_DRAW_LIMIT UI_CONTENT_WIDTH

static struct termios orig_termios;
static int terminal_raw_enabled = 0;

/*
 * Prints the available text commands.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Writes the help text to standard output.
 *
 * Behavior:
 * - Kept only for compatibility with older tests or fallback paths.
 * - The persistent TUI normally shows command hints directly in command mode.
 */
void print_help(void) {
    printf("\n");
    printf("Available commands:\n");
    printf("  register <nickname> <password>   register a new user\n");
    printf("  login <nickname> <password>      login with an existing user\n");
    printf("  ready                            mark yourself as ready\n");
    printf("  start                            start the session as owner\n");
    printf("  reset                            return a finished session to the lobby as owner\n");
    printf("  rank                             show ranking\n");
    printf("  users                            show connected users\n");
    printf("  local                            request local map\n");
    printf("  global                           request global map\n");
    printf("  w/a/s/d                          move in movement mode\n");
    printf("  quit                             disconnect from server\n");
    printf("  TAB                              switch movement/command mode\n");
    printf("\n");
}

/*
 * Clears the terminal screen.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Writes ANSI escape sequences to standard output.
 *
 * Behavior:
 * - Clears the whole screen.
 * - Moves the cursor to the top-left corner.
 */
void clear_screen(void) {
    printf("\033[2J");
    printf("\033[H");
}

/*
 * Enables raw terminal mode for single-key input.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 on success and -1 on failure.
 * - Updates internal terminal state tracking.
 *
 * Behavior:
 * - Disables canonical input and echo.
 * - Keeps read() usable for single-key commands.
 * - Fails gracefully if termios is not available.
 */
int terminal_set_raw(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
        return -1;
    }

    raw = orig_termios;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
        return -1;
    }

    terminal_raw_enabled = 1;
    return 0;
}

/*
 * Restores the terminal settings saved before raw mode.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Restores the terminal when raw mode is active.
 *
 * Behavior:
 * - Is safe to call multiple times.
 * - Does nothing if raw mode was never enabled.
 */
void terminal_restore(void) {
    if (terminal_raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        terminal_raw_enabled = 0;
    }
}

/*
 * Prints a text-based maze map.
 *
 * Input:
 * - title: label displayed above the map.
 * - map: flat character matrix.
 * - rows: number of map rows.
 * - cols: number of map columns.
 *
 * Output:
 * - Writes the map to standard output.
 *
 * Behavior:
 * - Uses UTF-8 box drawing characters.
 * - Replaces unknown cell values with CELL_HIDDEN.
 * - Preserves compatibility with older direct map-printing tests.
 */
void print_map(const char *title, const char *map, int rows, int cols) {
    printf("\n%s\n", title);
    printf("%s", BORDER_TOP_LEFT);

    for (int c = 0; c < cols; c++) {
        printf("%s", BORDER_HORIZONTAL);
    }

    printf("%s\n", BORDER_TOP_RIGHT);

    for (int r = 0; r < rows; r++) {
        printf("%s", BORDER_VERTICAL);

        for (int c = 0; c < cols; c++) {
            char cell = map[r * cols + c];

            switch (cell) {
                case CELL_WALL:
                case CELL_EMPTY:
                case CELL_OBJECT:
                case CELL_EXIT:
                case CELL_PLAYER:
                case CELL_HIDDEN:
                case CELL_PLAYER_HIDDEN:
                    printf("%c", cell);
                    break;
                default:
                    printf("%c", CELL_HIDDEN);
                    break;
            }
        }

        printf("%s\n", BORDER_VERTICAL);
    }

    printf("%s", BORDER_BOTTOM_LEFT);

    for (int c = 0; c < cols; c++) {
        printf("%s", BORDER_HORIZONTAL);
    }

    printf("%s\n", BORDER_BOTTOM_RIGHT);
}

/*
 * Prints connected users in a readable format.
 *
 * Input:
 * - users: array of user strings.
 * - count: number of users to print.
 *
 * Output:
 * - Writes a textual user list to standard output.
 *
 * Behavior:
 * - Preserves compatibility with older response tests.
 */
void print_users(char users[][MAX_NICK], int count) {
    printf("\nConnected users (%d):\n", count);

    for (int i = 0; i < count; i++) {
        printf("  - %s\n", users[i]);
    }

    printf("\n");
}

/*
 * Draws one bordered text line with padding.
 *
 * Input:
 * - text: content to display.
 *
 * Output:
 * - Writes a fixed-width row to standard output.
 *
 * Behavior:
 * - Truncates lines longer than UI_CONTENT_WIDTH.
 * - Pads shorter lines with spaces.
 * - Keeps all persistent UI rows aligned.
 */
static void draw_row(const char *text) {
    printf("%s %-*.*s %s\n",
           BORDER_VERTICAL,
           UI_CONTENT_WIDTH,
           UI_CONTENT_WIDTH,
           text != NULL ? text : "",
           BORDER_VERTICAL);
}

/*
 * Draws a horizontal border with UTF-8 box drawing characters.
 *
 * Input:
 * - left: left corner string.
 * - right: right corner string.
 *
 * Output:
 * - Writes one border line to standard output.
 *
 * Behavior:
 * - Uses UI_BORDER_WIDTH horizontal units.
 * - Matches the logical width used by draw_row().
 */
static void draw_border(const char *left, const char *right) {
    printf("%s", left);

    for (int i = 0; i < UI_BORDER_WIDTH; i++) {
        printf("%s", BORDER_HORIZONTAL);
    }

    printf("%s\n", right);
}

/*
 * Draws the command help area inside the persistent TUI.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Writes command hint rows to standard output.
 *
 * Behavior:
 * - Is shown automatically in COMMAND mode.
 * - Avoids printing help outside the persistent screen.
 * - Splits commands across multiple rows to avoid truncation.
 */
static void draw_command_help(void) {
    draw_row("");
    draw_row("Commands:");
    draw_row("  register <nickname> <password>   login <nickname> <password>");
    draw_row("  ready   start   reset   rank   users   local   global   quit");
    draw_row("Movement mode:");
    draw_row("  Press TAB, then use W/A/S/D to move, G for global, L for local, q to quit.");
}

/*
 * Draws the list of connected players.
 *
 * Input:
 * - state: current client state.
 *
 * Output:
 * - Writes the player list inside the current frame.
 *
 * Behavior:
 * - Shows a placeholder when no players are currently listed.
 * - Limits player rendering to MAX_USERS_DISPLAY entries through state->player_count.
 */
static void draw_players(const ClientState *state) {
    draw_row("Connected players:");

    if (state->player_count <= 0) {
        draw_row("  none");
        return;
    }

    for (int i = 0; i < state->player_count; i++) {
        char row[UI_CONTENT_WIDTH + 1];

        snprintf(row, sizeof(row), "  %.80s", state->player_list[i]);
        draw_row(row);
    }
}

/*
 * Draws the currently selected game map.
 *
 * Input:
 * - state: current client state.
 *
 * Output:
 * - Writes the local or global map inside the current frame.
 *
 * Behavior:
 * - Uses the global map when state->show_global is true.
 * - Uses the local map otherwise.
 * - Limits rendering to avoid exceeding the terminal frame.
 */
static void draw_selected_map(const ClientState *state) {
    const char *map;
    int rows;
    int cols;

    if (state->show_global) {
        draw_row("Global Masked Map [G toggles overlay | L returns to local]");
        map = state->global_map;
        rows = state->global_rows;
        cols = state->global_cols;
    } else {
        draw_row("Local Map [G toggles global overlay]");
        map = state->local_map;
        rows = state->local_rows;
        cols = state->local_cols;
    }

    for (int r = 0; r < rows && r < 24; r++) {
        char row[UI_CONTENT_WIDTH + 1];
        int pos = 0;

        for (int c = 0; c < cols && c < MAP_DRAW_LIMIT; c++) {
            row[pos++] = map[r * cols + c];
        }

        row[pos] = '\0';
        draw_row(row);
    }
}

/*
 * Draws the final ranking area.
 *
 * Input:
 * - state: current client state.
 *
 * Output:
 * - Writes final ranking information inside the current frame.
 *
 * Behavior:
 * - Shows a hint when no ranking has been received yet.
 * - Renders each stored ranking row.
 * - Always reminds the owner that reset can return the session to the lobby.
 */
static void draw_results(const ClientState *state) {
    draw_row("Session finished - Results");

    if (state->rank_count == 0) {
        draw_row("Type rank to view results.");
    } else {
        for (int i = 0; i < state->rank_count; i++) {
            draw_row(state->rank_lines[i]);
        }
    }

    draw_row("");
    draw_row(state->is_owner ? "Owner command: reset returns all connected players to the lobby." :
                               "Wait for the owner to reset the session back to the lobby.");
}

/*
 * Draws the current input area.
 *
 * Input:
 * - state: current client state.
 * - mode: string representation of the current input mode.
 *
 * Output:
 * - Writes status, input buffer and mode information.
 *
 * Behavior:
 * - COMMAND mode shows the editable command buffer.
 * - MOVEMENT mode shows compact key bindings.
 */
static void draw_input_area(const ClientState *state, const char *mode) {
    draw_row("");
    draw_row(state->status_message);

    if (state->mode == COMMAND) {
        char input_line[MAX_MSG + 16];

        snprintf(input_line, sizeof(input_line), "> %s", state->input_buffer);
        draw_row(input_line);
    } else {
        draw_row("Keys: WASD move | G global | L local | TAB command | q quit");
    }

    draw_row(mode);
}

/*
 * Draws the current lobby, game or results screen.
 *
 * Input:
 * - state: current client state to render.
 *
 * Output:
 * - Writes a complete screen frame to standard output.
 *
 * Behavior:
 * - Clears the screen before drawing.
 * - Shows a lobby before START, maps during PLAYING and ranking when FINISHED.
 * - Shows command help automatically in COMMAND mode.
 * - Uses only printf and ANSI escape sequences, not ncurses.
 */
void draw_screen(const ClientState *state) {
    const char *mode;

    if (state == NULL) {
        return;
    }

    clear_screen();

    mode = (state->mode == COMMAND) ? "COMMAND" : "MOVEMENT";

    draw_border(BORDER_TOP_LEFT, BORDER_TOP_RIGHT);

    if (state->session_state == CLIENT_LOBBY) {
        draw_row("Lobby - Waiting for players");
        draw_row(state->is_owner ? "You are the owner. Type start to begin." :
                                   "Type ready to mark ready. Owner can start.");
        draw_players(state);
    } else if (state->session_state == CLIENT_PLAYING) {
        draw_selected_map(state);
    } else {
        draw_results(state);
    }

    if (state->mode == COMMAND) {
        draw_command_help();
    }

    draw_input_area(state, mode);

    draw_border(BORDER_BOTTOM_LEFT, BORDER_BOTTOM_RIGHT);
    fflush(stdout);
}
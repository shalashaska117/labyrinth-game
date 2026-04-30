#include "ui.h"
#include "../common/constants.h"

#include <stdio.h>

/*
 * Prints the available commands.
 */
void print_help(void) {
    printf("\n");
    printf("Available commands:\n");
    printf("  register <nickname> <password>   register a new user\n");
    printf("  login <nickname> <password>      login with an existing user\n");
    printf("  w                                move up\n");
    printf("  s                                move down\n");
    printf("  a                                move left\n");
    printf("  d                                move right\n");
    printf("  local                            show local map\n");
    printf("  global                           show global masked map\n");
    printf("  users                            show connected users\n");
    printf("  quit                             disconnect from server\n");
    printf("  help                             show this help message\n");
    printf("\n");
}

/*
 * Clears the terminal screen.
 */
void clear_screen(void) {
    printf("\033[2J");
    printf("\033[H");
}

/*
 * Prints a text-based maze map.
 */
void print_map(const char *title, const char *map, int rows, int cols) {
    int r;
    int c;

    printf("\n%s\n", title);

    for (c = 0; c < cols + 2; c++) {
        printf("-");
    }
    printf("\n");

    for (r = 0; r < rows; r++) {
        printf("|");

        for (c = 0; c < cols; c++) {
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

        printf("|\n");
    }

    for (c = 0; c < cols + 2; c++) {
        printf("-");
    }
    printf("\n");
}

/*
 * Prints connected users in a readable format.
 */
void print_users(char users[][MAX_NICK], int count) {
    int i;

    printf("\nConnected users (%d):\n", count);

    for (i = 0; i < count; i++) {
        printf("  - %s\n", users[i]);
    }

    printf("\n");
}

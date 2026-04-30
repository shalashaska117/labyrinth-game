#ifndef UI_H
#define UI_H

#include "../common/protocol.h"

/*
 * Prints all commands available to the player.
 */
void print_help(void);

/*
 * Clears the terminal screen using ANSI escape sequences.
 */
void clear_screen(void);

/*
 * Prints a map received from the server.
 *
 * title: text displayed above the map.
 * map: flat character array representing the matrix.
 * rows: number of rows.
 * cols: number of columns.
 */
void print_map(const char *title, const char *map, int rows, int cols);

/*
 * Prints a formatted list of connected users.
 *
 * users: array of strings containing usernames.
 * count: number of users contained in the array.
 */
void print_users(char users[][MAX_NICK], int count);

#endif

#ifndef CONSTANTS_H
#define CONSTANTS_H

/*
 * Size of the maze matrix.
 */
#define MAP_WIDTH 21
#define MAP_HEIGHT 21

/*
 * Number of cells revealed around the player.
 * Radius 2 means that the local visible area is 5x5.
 */
#define VIEW_RADIUS 2

/*
 * Number of objects placed randomly in the maze.
 */
#define OBJECT_COUNT 15

/*
 * Full session duration in seconds.
 */
#define SESSION_DURATION 300

/*
 * Interval in seconds between periodic global map broadcasts.
 */
#define T_INTERVAL 5

/*
 * Maximum size for protocol buffers.
 */
#define BUFFER_SIZE 4096

/*
 * Maximum number of users printed by the client when receiving
 * a USERS response from the server.
 */
#define MAX_USERS_DISPLAY 64

/*
 * Symbols used to represent the maze in text mode.
 */
#define CELL_WALL '#'
#define CELL_EMPTY ' '
#define CELL_OBJECT '*'
#define CELL_EXIT 'E'
#define CELL_PLAYER 'P'
#define CELL_HIDDEN '?'
#define CELL_PLAYER_HIDDEN '@'

#endif

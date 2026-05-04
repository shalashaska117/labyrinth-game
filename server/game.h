#ifndef GAME_H
#define GAME_H

#include "../common/constants.h"

typedef struct { int x, y; } position_t;

typedef enum { TILE_WALL, TILE_EMPTY, TILE_OBJECT, TILE_EXIT } tile_t;

typedef struct {
    tile_t tile;
    int has_object;
    int is_exit;
} cell_t;

int  game_init(cell_t maze[MAP_HEIGHT][MAP_WIDTH]);
int  game_is_walkable(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y);
int  game_move(cell_t maze[MAP_HEIGHT][MAP_WIDTH], position_t *pos, char dir);
int  game_pickup(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y);
int  game_is_exit(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y);
void game_mark_visible(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                       int visible[MAP_HEIGHT][MAP_WIDTH], int px, int py);
void game_build_local_view(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                           char *out, size_t out_size,
                           int px, int py, int visible[MAP_HEIGHT][MAP_WIDTH]);
void game_build_global_view(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                            char *out, size_t out_size,
                            int visible[MAP_HEIGHT][MAP_WIDTH]);

#endif

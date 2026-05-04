#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Checks for out of bounds
 * */
static int inside(int x, int y) {
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

static void shuffle_directions(int dirs[4][2]) {
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tx = dirs[i][0]; dirs[i][0] = dirs[j][0]; dirs[j][0] = tx;
        int ty = dirs[i][1]; dirs[i][1] = dirs[j][1]; dirs[j][1] = ty;
    }
}

static void carve(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    int directions[4][2] = {{0, -2}, {0, 2}, {-2, 0}, {2, 0}}; //down, up, left, right
    shuffle_directions(directions); //at each recurse, randomically change dirs to carve the labyrinth

    for (int i = 0; i < 4; i++) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];

        if (inside(nx, ny) && maze[ny][nx].tile == TILE_WALL) {
            maze[ny][nx].tile = TILE_EMPTY; //carve with offset
	
						// the algorithms work by jumping 2 cells at a time, dividing the curr coordinate by 2
						// find the grid in the middle
            maze[y + directions[i][1] / 2][x + directions[i][0] / 2].tile = TILE_EMPTY; //carve in between 
            carve(maze, nx, ny);
        }
    }
}

/*
 * Will randomically place the exit at the border of the labyrinth
 * */
static void place_exit(cell_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    int ex, ey;
    do {
        int side = rand() % 4;
        switch (side) {
            case 0: ex = rand() % MAP_WIDTH;  ey = 0;                break;
            case 1: ex = rand() % MAP_WIDTH;  ey = MAP_HEIGHT - 1;   break;
            case 2: ex = 0;                   ey = rand() % MAP_HEIGHT; break;
            case 3: ex = MAP_WIDTH - 1;       ey = rand() % MAP_HEIGHT; break;
        }
    } while (maze[ey][ex].tile != TILE_EMPTY);

    maze[ey][ex].tile = TILE_EXIT;
    maze[ey][ex].is_exit = 1;
}

int game_init(cell_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    srand((unsigned)time(NULL));

    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++) {
            maze[y][x].tile = TILE_WALL;
            maze[y][x].has_object = 0;
            maze[y][x].is_exit = 0;
        }

    maze[1][1].tile = TILE_EMPTY;
    carve(maze, 1, 1);

    place_exit(maze);

    int placed = 0;
    while (placed < OBJECT_COUNT) { //place objects
        int x = rand() % (MAP_WIDTH - 2) + 1;
        int y = rand() % (MAP_HEIGHT - 2) + 1;
        if (maze[y][x].tile == TILE_EMPTY &&
            !maze[y][x].is_exit && !maze[y][x].has_object) {
            maze[y][x].tile = TILE_OBJECT;
            maze[y][x].has_object = 1;
            placed++;
        }
    }

    return 0;
}

int game_is_walkable(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    return inside(x, y) && maze[y][x].tile != TILE_WALL;
}

int game_move(cell_t maze[MAP_HEIGHT][MAP_WIDTH], position_t *pos, char dir) {
    int dx = 0, dy = 0;

    switch (dir) {
        case 'w': case 'W': dy = -1; break;
        case 's': case 'S': dy =  1; break;
        case 'a': case 'A': dx = -1; break;
        case 'd': case 'D': dx =  1; break;
        default: return -1;
    }

    int nx = pos->x + dx;
    int ny = pos->y + dy;

    if (!game_is_walkable(maze, nx, ny))
        return -1;

    pos->x = nx;
    pos->y = ny;
    return 0;
}

int game_pickup(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    if (!inside(x, y) || !maze[y][x].has_object)
        return 0;

    maze[y][x].has_object = 0;
    maze[y][x].tile = TILE_EMPTY;

    return 1;
}

int game_is_exit(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    return inside(x, y) && maze[y][x].is_exit;
}

/*
 * px, py - player position (center of the view)
 * dx, dy - delta (offset) from the player
 * wx, wy - world position (the actual cell coordinate being processed)
 * */
void game_mark_visible(int visible[MAP_HEIGHT][MAP_WIDTH],
                       int px,
											 int py) 
{
    for (int dy = -VIEW_RADIUS; dy <= VIEW_RADIUS; dy++)
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++) {
            int wx = px + dx;
            int wy = py + dy;
            if (inside(wx, wy))
                visible[wy][wx] = 1;
        }
}

static char cell_char(cell_t *c, int vis, int player) {
    if (player) return CELL_PLAYER;
    if (!vis)   return CELL_HIDDEN;
    if (c->is_exit)    return CELL_EXIT;
    if (c->has_object) return CELL_OBJECT;
    if (c->tile == TILE_WALL) return CELL_WALL;
    return CELL_EMPTY;
}

/*
 * px, py - player position (center of the view)
 * dx, dy - delta (offset) from the player
 * wx, wy - world position (the actual cell coordinate being processed)
 * */
void game_build_local_view(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                           char *out, 
													 size_t out_size,
                           int px, 
													 int py,
                           int visible[MAP_HEIGHT][MAP_WIDTH])
{
    int rows = VIEW_RADIUS * 2 + 1;
    int cols = VIEW_RADIUS * 2 + 1;
    char grid[rows][cols];

    for (int dy = -VIEW_RADIUS; dy <= VIEW_RADIUS; dy++)
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++) {
            int wx = px + dx, wy = py + dy;
            int is_player = (dx == 0 && dy == 0);
            if (inside(wx, wy))
                grid[dy + VIEW_RADIUS][dx + VIEW_RADIUS] = cell_char(&maze[wy][wx], 
																																		 visible[wy][wx], 
																																		 is_player);
            else
                grid[dy + VIEW_RADIUS][dx + VIEW_RADIUS] = CELL_WALL;
        }

	// the remaining section will flatten the 2d map into a string to be decoded by the client.
    char body[(rows * cols) + rows + 1];
    int pos = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++)
            body[pos++] = grid[r][c];
        body[pos++] = '\n';
    }
    body[pos] = '\0';

    snprintf(out, out_size, "MAP LOCAL %d %d\n%sEND\n", rows, cols, body);
}

/*
 * Sends map as a flat string to the client
 * */
void game_build_global_view(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                            char *out, size_t out_size,
                            int visible[MAP_HEIGHT][MAP_WIDTH])
{
    char body[MAP_HEIGHT * (MAP_WIDTH + 1) + 1];
    int pos = 0;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++)
            body[pos++] = cell_char(&maze[y][x], visible[y][x], 0);
        body[pos++] = '\n';
    }

    body[pos] = '\0';

    snprintf(out, out_size, "MAP GLOBAL %d %d\n%sEND\n", MAP_WIDTH, MAP_HEIGHT, body);
}

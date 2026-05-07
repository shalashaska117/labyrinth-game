#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Checks whether a maze coordinate is inside the matrix.
 *
 * Input:
 * - x: horizontal coordinate.
 * - y: vertical coordinate.
 *
 * Output:
 * - Returns 1 if the coordinate is inside the maze, 0 otherwise.
 *
 * Behavior:
 * - Uses MAP_WIDTH and MAP_HEIGHT as exclusive upper bounds.
 */
static int inside(int x, int y) {
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

/*
 * Randomly shuffles the four maze carving directions.
 *
 * Input:
 * - dirs: four two-dimensional direction vectors.
 *
 * Output:
 * - Modifies dirs in place.
 *
 * Behavior:
 * - Uses a Fisher-Yates style shuffle.
 * - Keeps the recursive maze generator from always producing the same shape.
 */
static void shuffle_directions(int dirs[4][2]) {
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tx = dirs[i][0];
        int ty = dirs[i][1];

        dirs[i][0] = dirs[j][0];
        dirs[i][1] = dirs[j][1];
        dirs[j][0] = tx;
        dirs[j][1] = ty;
    }
}

/*
 * Recursively carves passages in the maze.
 *
 * Input:
 * - maze: maze matrix to modify.
 * - x: current carving x coordinate.
 * - y: current carving y coordinate.
 *
 * Output:
 * - Modifies maze by turning selected walls into empty cells.
 *
 * Behavior:
 * - Jumps two cells at a time to preserve walls between corridors.
 * - Opens the intermediate wall when moving to a new cell.
 */
static void carve(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    int directions[4][2] = {{0, -2}, {0, 2}, {-2, 0}, {2, 0}};

    shuffle_directions(directions);

    for (int i = 0; i < 4; i++) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];

        if (inside(nx, ny) && maze[ny][nx].tile == TILE_WALL) {
            maze[ny][nx].tile = TILE_EMPTY;
            maze[y + directions[i][1] / 2][x + directions[i][0] / 2].tile = TILE_EMPTY;
            carve(maze, nx, ny);
        }
    }
}

/*
 * Places the exit near one of the maze borders.
 *
 * Input:
 * - maze: maze matrix to modify.
 *
 * Output:
 * - Modifies one internal border-adjacent cell as the exit.
 * - Opens the corresponding outer border cell.
 *
 * Behavior:
 * - Selects one side randomly.
 * - Uses odd coordinates so the exit is connected to carved corridors.
 */
static void place_exit(cell_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    int side = rand() % 4;
    int ex = 1;
    int ey = 1;

    switch (side) {
        case 0:
            ex = (rand() % ((MAP_WIDTH - 3) / 2)) * 2 + 1;
            ey = 1;
            break;
        case 1:
            ex = (rand() % ((MAP_WIDTH - 3) / 2)) * 2 + 1;
            ey = MAP_HEIGHT - 2;
            break;
        case 2:
            ex = 1;
            ey = (rand() % ((MAP_HEIGHT - 3) / 2)) * 2 + 1;
            break;
        default:
            ex = MAP_WIDTH - 2;
            ey = (rand() % ((MAP_HEIGHT - 3) / 2)) * 2 + 1;
            break;
    }

    maze[ey][ex].tile = TILE_EXIT;
    maze[ey][ex].is_exit = 1;

    switch (side) {
        case 0:
            maze[0][ex].tile = TILE_EMPTY;
            break;
        case 1:
            maze[MAP_HEIGHT - 1][ex].tile = TILE_EMPTY;
            break;
        case 2:
            maze[ey][0].tile = TILE_EMPTY;
            break;
        default:
            maze[ey][MAP_WIDTH - 1].tile = TILE_EMPTY;
            break;
    }
}

/*
 * Initializes a new random labyrinth.
 *
 * Input:
 * - maze: maze matrix to initialize.
 *
 * Output:
 * - Returns 0 after initialization.
 * - Rewrites all cells, objects and exit metadata.
 *
 * Behavior:
 * - Starts with a full wall matrix.
 * - Carves passages from coordinate (1, 1).
 * - Places one exit and OBJECT_COUNT collectible objects.
 */
int game_init(cell_t maze[MAP_HEIGHT][MAP_WIDTH]) {
    int placed = 0;

    srand((unsigned)time(NULL));

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            maze[y][x].tile = TILE_WALL;
            maze[y][x].has_object = 0;
            maze[y][x].is_exit = 0;
        }
    }

    maze[1][1].tile = TILE_EMPTY;
    carve(maze, 1, 1);
    place_exit(maze);

    while (placed < OBJECT_COUNT) {
        int x = rand() % (MAP_WIDTH - 2) + 1;
        int y = rand() % (MAP_HEIGHT - 2) + 1;

        if (maze[y][x].tile == TILE_EMPTY && !maze[y][x].is_exit && !maze[y][x].has_object) {
            maze[y][x].tile = TILE_OBJECT;
            maze[y][x].has_object = 1;
            placed++;
        }
    }

    return 0;
}

/*
 * Checks whether a player can enter a maze cell.
 *
 * Input:
 * - maze: current maze matrix.
 * - x: target x coordinate.
 * - y: target y coordinate.
 *
 * Output:
 * - Returns 1 if the cell can be entered, 0 otherwise.
 *
 * Behavior:
 * - Rejects coordinates outside the maze.
 * - Rejects wall cells.
 */
int game_is_walkable(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    return inside(x, y) && maze[y][x].tile != TILE_WALL;
}

/*
 * Moves a player position by one cell.
 *
 * Input:
 * - maze: current maze matrix.
 * - pos: player position to update.
 * - dir: movement direction encoded as w, a, s or d.
 *
 * Output:
 * - Returns 0 if the movement succeeds and -1 otherwise.
 * - Modifies pos only when the target cell is walkable.
 *
 * Behavior:
 * - Supports both lowercase and uppercase movement characters.
 * - Rejects unknown directions and blocked cells.
 */
int game_move(cell_t maze[MAP_HEIGHT][MAP_WIDTH], position_t *pos, char dir) {
    int dx = 0;
    int dy = 0;
    int nx;
    int ny;

    if (pos == NULL) {
        return -1;
    }

    switch (dir) {
        case 'w':
        case 'W':
            dy = -1;
            break;
        case 's':
        case 'S':
            dy = 1;
            break;
        case 'a':
        case 'A':
            dx = -1;
            break;
        case 'd':
        case 'D':
            dx = 1;
            break;
        default:
            return -1;
    }

    nx = pos->x + dx;
    ny = pos->y + dy;

    if (!game_is_walkable(maze, nx, ny)) {
        return -1;
    }

    pos->x = nx;
    pos->y = ny;
    return 0;
}

/*
 * Collects an object from a maze cell.
 *
 * Input:
 * - maze: current maze matrix.
 * - x: object x coordinate.
 * - y: object y coordinate.
 *
 * Output:
 * - Returns 1 if an object was collected and 0 otherwise.
 * - Removes the object from the maze when present.
 *
 * Behavior:
 * - Ignores invalid coordinates.
 * - Converts the object cell back to an empty cell.
 */
int game_pickup(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    if (!inside(x, y) || !maze[y][x].has_object) {
        return 0;
    }

    maze[y][x].has_object = 0;
    maze[y][x].tile = TILE_EMPTY;
    return 1;
}

/*
 * Checks whether a maze cell is the exit.
 *
 * Input:
 * - maze: current maze matrix.
 * - x: x coordinate to inspect.
 * - y: y coordinate to inspect.
 *
 * Output:
 * - Returns 1 if the coordinate is the exit and 0 otherwise.
 *
 * Behavior:
 * - Rejects invalid coordinates before reading the matrix.
 */
int game_is_exit(cell_t maze[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    return inside(x, y) && maze[y][x].is_exit;
}

/*
 * Marks cells visible around a player.
 *
 * Input:
 * - visible: player-specific visibility matrix.
 * - px: player x coordinate.
 * - py: player y coordinate.
 *
 * Output:
 * - Modifies visible by marking nearby cells as discovered.
 *
 * Behavior:
 * - Reveals a square area centered on the player.
 * - Ignores out-of-bounds cells at the map edges.
 */
void game_mark_visible(int visible[MAP_HEIGHT][MAP_WIDTH], int px, int py) {
    for (int dy = -VIEW_RADIUS; dy <= VIEW_RADIUS; dy++) {
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++) {
            int wx = px + dx;
            int wy = py + dy;

            if (inside(wx, wy)) {
                visible[wy][wx] = 1;
            }
        }
    }
}

/*
 * Converts one maze cell into a printable character.
 *
 * Input:
 * - c: cell to render.
 * - visible: 1 if the cell is visible to the player.
 * - player: 1 if the player is currently on this cell.
 *
 * Output:
 * - Returns the character that represents the cell.
 *
 * Behavior:
 * - Gives the player marker priority over hidden and real cell content.
 * - Hides undiscovered cells with CELL_HIDDEN.
 */
static char cell_char(cell_t *c, int visible, int player) {
    if (player) {
        return CELL_PLAYER;
    }

    if (!visible) {
        return CELL_HIDDEN;
    }

    if (c->is_exit) {
        return CELL_EXIT;
    }

    if (c->has_object) {
        return CELL_OBJECT;
    }

    if (c->tile == TILE_WALL) {
        return CELL_WALL;
    }

    return CELL_EMPTY;
}

/*
 * Builds the local map response for one player.
 *
 * Input:
 * - maze: current maze matrix.
 * - out: output protocol buffer.
 * - out_size: size of the output buffer.
 * - px: player x coordinate.
 * - py: player y coordinate.
 * - visible: player-specific visibility matrix.
 *
 * Output:
 * - Writes a MAP LOCAL multi-line protocol response into out.
 *
 * Behavior:
 * - Produces a square view centered on the player.
 * - Uses END to terminate the multi-line response.
 */
void game_build_local_view(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                           char *out,
                           size_t out_size,
                           int px,
                           int py,
                           int visible[MAP_HEIGHT][MAP_WIDTH]) {
    int rows = VIEW_RADIUS * 2 + 1;
    int cols = VIEW_RADIUS * 2 + 1;
    char body[(VIEW_RADIUS * 2 + 1) * (VIEW_RADIUS * 2 + 2) + 1];
    int pos = 0;

    if (out == NULL || out_size == 0) {
        return;
    }

    for (int dy = -VIEW_RADIUS; dy <= VIEW_RADIUS; dy++) {
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++) {
            int wx = px + dx;
            int wy = py + dy;
            int is_player = (dx == 0 && dy == 0);

            if (inside(wx, wy)) {
                body[pos++] = cell_char(&maze[wy][wx], visible[wy][wx], is_player);
            } else {
                body[pos++] = CELL_WALL;
            }
        }

        body[pos++] = '\n';
    }

    body[pos] = '\0';
    snprintf(out, out_size, "MAP LOCAL %d %d\n%sEND\n", rows, cols, body);
}

/*
 * Builds the global masked map response for one player.
 *
 * Input:
 * - maze: current maze matrix.
 * - out: output protocol buffer.
 * - out_size: size of the output buffer.
 * - px: player x coordinate.
 * - py: player y coordinate.
 * - visible: player-specific visibility matrix.
 *
 * Output:
 * - Writes a MAP GLOBAL multi-line protocol response into out.
 *
 * Behavior:
 * - Renders the full maze using the player's discovered cells.
 * - Gives CELL_PLAYER priority even if the current cell would otherwise be hidden.
 * - Terminates the response with END.
 */
void game_build_global_view(cell_t maze[MAP_HEIGHT][MAP_WIDTH],
                            char *out,
                            size_t out_size,
                            int px,
                            int py,
                            int visible[MAP_HEIGHT][MAP_WIDTH]) {
    char body[MAP_HEIGHT * (MAP_WIDTH + 1) + 1];
    int pos = 0;

    if (out == NULL || out_size == 0) {
        return;
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int is_player = (x == px && y == py);
            body[pos++] = cell_char(&maze[y][x], visible[y][x], is_player);
        }

        body[pos++] = '\n';
    }

    body[pos] = '\0';
    snprintf(out, out_size, "MAP GLOBAL %d %d\n%sEND\n", MAP_HEIGHT, MAP_WIDTH, body);
}

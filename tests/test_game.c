#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * White-box test support for server/game.c.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Exposes static helper functions only inside this test translation unit.
 *
 * Behavior:
 * - Replaces rand(), srand() and time() with deterministic test doubles.
 * - Includes server/game.c directly so static helpers can be tested without
 *   changing production headers.
 */
#define static
#define rand test_rand
#define srand test_srand
#define time test_time

static unsigned int test_rand_state = 1;
static int test_rand_sequence[256];
static int test_rand_sequence_len = 0;
static int test_rand_sequence_pos = 0;

static void test_set_rand_sequence(const int *values, int count) {
    int i;

    test_rand_sequence_len = count;
    test_rand_sequence_pos = 0;

    for (i = 0; i < count && i < (int)(sizeof(test_rand_sequence) / sizeof(test_rand_sequence[0])); i++) {
        test_rand_sequence[i] = values[i];
    }
}

static int test_rand(void) {
    if (test_rand_sequence_pos < test_rand_sequence_len) {
        return test_rand_sequence[test_rand_sequence_pos++];
    }

    test_rand_state = test_rand_state * 1103515245u + 12345u;
    return (int)((test_rand_state / 65536u) % 32768u);
}

static void test_srand(unsigned int seed) {
    test_rand_state = seed ? seed : 1u;
    test_rand_sequence_len = 0;
    test_rand_sequence_pos = 0;
}

static time_t test_time(time_t *out) {
    if (out != NULL) {
        *out = (time_t)12345;
    }

    return (time_t)12345;
}

#include "../server/game.c"

#undef time
#undef srand
#undef rand
#undef static

/*
 * Clears a maze to a known state.
 *
 * Input:
 * - maze: maze matrix to initialize.
 * - tile: tile type to assign to every cell.
 *
 * Output:
 * - Updates every maze cell.
 *
 * Behavior:
 * - Clears object and exit metadata.
 * - Makes individual tests deterministic.
 */
static void fill_maze(cell_t maze[MAP_HEIGHT][MAP_WIDTH], tile_t tile) {
    int y;
    int x;

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            maze[y][x].tile = tile;
            maze[y][x].has_object = 0;
            maze[y][x].is_exit = 0;
        }
    }
}

/*
 * Fails the test with a diagnostic message.
 *
 * Input:
 * - message: failure explanation.
 *
 * Output:
 * - Returns 0 to the caller.
 *
 * Behavior:
 * - Keeps the test style consistent with the other project tests.
 */
static int fail(const char *message) {
    printf("FAILED: %s\n", message);
    return 0;
}

/*
 * Tests coordinate bounds and walkability.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers inside() and game_is_walkable() with valid and invalid positions.
 */
static int test_bounds_and_walkability(void) {
    cell_t maze[MAP_HEIGHT][MAP_WIDTH];

    fill_maze(maze, TILE_EMPTY);
    maze[2][2].tile = TILE_WALL;

    if (!inside(0, 0) || !inside(MAP_WIDTH - 1, MAP_HEIGHT - 1)) {
        return fail("inside rejected valid coordinates");
    }

    if (inside(-1, 0) || inside(0, -1) || inside(MAP_WIDTH, 0) || inside(0, MAP_HEIGHT)) {
        return fail("inside accepted invalid coordinates");
    }

    if (!game_is_walkable(maze, 1, 1) || game_is_walkable(maze, 2, 2)) {
        return fail("walkability result is invalid");
    }

    if (game_is_walkable(maze, -1, 1) || game_is_walkable(maze, 1, MAP_HEIGHT)) {
        return fail("out-of-bounds cell should not be walkable");
    }

    printf("PASSED: bounds and walkability\n");
    return 1;
}

/*
 * Tests all movement directions and blocked movement.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers lowercase and uppercase directions.
 * - Covers invalid direction, null position and blocked target cases.
 */
static int test_movement(void) {
    cell_t maze[MAP_HEIGHT][MAP_WIDTH];
    position_t pos;

    fill_maze(maze, TILE_EMPTY);

    pos.x = 5;
    pos.y = 5;

    if (game_move(maze, &pos, 'w') != 0 || pos.x != 5 || pos.y != 4) return fail("move up failed");
    if (game_move(maze, &pos, 'S') != 0 || pos.x != 5 || pos.y != 5) return fail("move down failed");
    if (game_move(maze, &pos, 'a') != 0 || pos.x != 4 || pos.y != 5) return fail("move left failed");
    if (game_move(maze, &pos, 'D') != 0 || pos.x != 5 || pos.y != 5) return fail("move right failed");

    maze[5][6].tile = TILE_WALL;
    if (game_move(maze, &pos, 'd') == 0 || pos.x != 5 || pos.y != 5) return fail("blocked movement changed the position");
    if (game_move(maze, &pos, '?') == 0) return fail("invalid direction was accepted");
    if (game_move(maze, NULL, 'w') == 0) return fail("null position was accepted");

    printf("PASSED: movement\n");
    return 1;
}

/*
 * Tests object pickup and exit detection.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers object collection, repeated pickup and invalid coordinates.
 * - Covers exit detection for true, false and invalid coordinates.
 */
static int test_pickup_and_exit(void) {
    cell_t maze[MAP_HEIGHT][MAP_WIDTH];

    fill_maze(maze, TILE_EMPTY);

    maze[3][4].tile = TILE_OBJECT;
    maze[3][4].has_object = 1;

    if (!game_pickup(maze, 4, 3)) return fail("object was not collected");
    if (maze[3][4].has_object || maze[3][4].tile != TILE_EMPTY) return fail("object cell was not cleared");
    if (game_pickup(maze, 4, 3)) return fail("same object was collected twice");
    if (game_pickup(maze, -1, 3) || game_pickup(maze, 3, MAP_HEIGHT)) return fail("invalid pickup coordinates accepted");

    maze[7][8].tile = TILE_EXIT;
    maze[7][8].is_exit = 1;

    if (!game_is_exit(maze, 8, 7)) return fail("exit was not detected");
    if (game_is_exit(maze, 1, 1) || game_is_exit(maze, MAP_WIDTH, 1)) return fail("non-exit or invalid coordinate detected as exit");

    printf("PASSED: pickup and exit\n");
    return 1;
}

/*
 * Tests visibility marking near the center and near borders.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers normal visibility expansion.
 * - Covers out-of-bounds skipping at the top-left corner.
 */
static int test_visibility(void) {
    int visible[MAP_HEIGHT][MAP_WIDTH];

    memset(visible, 0, sizeof(visible));
    game_mark_visible(visible, 5, 5);

    if (!visible[5][5] || !visible[3][3] || !visible[7][7]) return fail("center visibility area was not marked");
    if (visible[2][2] || visible[8][8]) return fail("visibility exceeded expected radius");

    memset(visible, 0, sizeof(visible));
    game_mark_visible(visible, 0, 0);

    if (!visible[0][0] || !visible[2][2]) return fail("border visibility area was not marked");

    printf("PASSED: visibility\n");
    return 1;
}

/*
 * Tests printable cell conversion.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers player priority, hidden cells, exit, object, wall and empty rendering.
 */
static int test_cell_rendering(void) {
    cell_t cell;

    memset(&cell, 0, sizeof(cell));
    cell.tile = TILE_EMPTY;

    if (cell_char(&cell, 0, 1) != CELL_PLAYER) return fail("player marker did not have priority");
    if (cell_char(&cell, 0, 0) != CELL_HIDDEN) return fail("hidden empty cell did not render as hidden");

    cell.is_exit = 1;
    if (cell_char(&cell, 1, 0) != CELL_EXIT) return fail("exit did not render correctly");

    cell.is_exit = 0;
    cell.has_object = 1;
    if (cell_char(&cell, 1, 0) != CELL_OBJECT) return fail("object did not render correctly");

    cell.has_object = 0;
    cell.tile = TILE_WALL;
    if (cell_char(&cell, 1, 0) != CELL_WALL) return fail("wall did not render correctly");

    cell.tile = TILE_EMPTY;
    if (cell_char(&cell, 1, 0) != CELL_EMPTY) return fail("empty cell did not render correctly");

    printf("PASSED: cell rendering\n");
    return 1;
}

/*
 * Tests local and global map response building.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Covers local view construction, global view construction and null output guards.
 * - Verifies hidden, wall, player, object and exit characters in protocol output.
 */
static int test_map_builders(void) {
    cell_t maze[MAP_HEIGHT][MAP_WIDTH];
    int visible[MAP_HEIGHT][MAP_WIDTH];
    char out[BUFFER_SIZE];

    fill_maze(maze, TILE_EMPTY);
    memset(visible, 0, sizeof(visible));

    maze[0][0].tile = TILE_WALL;
    maze[1][2].tile = TILE_OBJECT;
    maze[1][2].has_object = 1;
    maze[2][1].tile = TILE_EXIT;
    maze[2][1].is_exit = 1;

    visible[1][1] = 1;
    visible[1][2] = 1;
    visible[2][1] = 1;
    visible[0][0] = 1;

    game_build_local_view(maze, out, sizeof(out), 1, 1, visible);
    if (strstr(out, "MAP LOCAL") == NULL || strstr(out, "END") == NULL || strchr(out, CELL_PLAYER) == NULL) return fail("local map response is incomplete");
    if (strchr(out, CELL_OBJECT) == NULL || strchr(out, CELL_EXIT) == NULL) return fail("local map did not include visible object and exit");

    game_build_global_view(maze, out, sizeof(out), 1, 1, visible);
    if (strstr(out, "MAP GLOBAL") == NULL || strstr(out, "END") == NULL || strchr(out, CELL_PLAYER) == NULL) return fail("global map response is incomplete");
    if (strchr(out, CELL_HIDDEN) == NULL) return fail("global map did not include hidden cells");

    game_build_local_view(maze, NULL, sizeof(out), 1, 1, visible);
    game_build_local_view(maze, out, 0, 1, 1, visible);
    game_build_global_view(maze, NULL, sizeof(out), 1, 1, visible);
    game_build_global_view(maze, out, 0, 1, 1, visible);

    printf("PASSED: map builders\n");
    return 1;
}

/*
 * Tests deterministic exit placement branches.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Drives place_exit() through all four side branches using deterministic rand values.
 * - Verifies that an exit marker is produced.
 */
static int test_exit_placement_branches(void) {
    cell_t maze[MAP_HEIGHT][MAP_WIDTH];

    for (int side = 0; side < 4; side++) {
        int values[2] = {side, 0};
        int found_exit = 0;

        fill_maze(maze, TILE_WALL);
        test_set_rand_sequence(values, 2);
        place_exit(maze);

        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (maze[y][x].is_exit) found_exit = 1;
            }
        }

        if (!found_exit) return fail("exit placement did not create an exit");
    }

    printf("PASSED: exit placement branches\n");
    return 1;
}

/*
 * Tests random maze initialization.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 1 on success and 0 on failure.
 *
 * Behavior:
 * - Calls game_init() with deterministic random functions.
 * - Verifies that start is walkable, one exit exists and OBJECT_COUNT objects are placed.
 */
static int test_game_init(void) {
    cell_t maze[MAP_HEIGHT][MAP_WIDTH];
    int exits = 0;
    int objects = 0;

    if (game_init(maze) != 0) return fail("game_init failed");
    if (!game_is_walkable(maze, 1, 1)) return fail("starting cell is not walkable");

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (maze[y][x].is_exit) exits++;
            if (maze[y][x].has_object) objects++;
        }
    }

    if (exits != 1) return fail("game_init did not place exactly one exit");
    if (objects != OBJECT_COUNT) return fail("game_init did not place the expected number of objects");

    printf("PASSED: game initialization\n");
    return 1;
}

/*
 * Unit test entry point for server game logic.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 if all tests pass and 1 otherwise.
 *
 * Behavior:
 * - Exercises deterministic, non-network server game logic.
 * - Uses no external testing framework.
 */
int main(void) {
    int passed = 0;
    int total = 0;

    total++; passed += test_bounds_and_walkability();
    total++; passed += test_movement();
    total++; passed += test_pickup_and_exit();
    total++; passed += test_visibility();
    total++; passed += test_cell_rendering();
    total++; passed += test_map_builders();
    total++; passed += test_exit_placement_branches();
    total++; passed += test_game_init();

    printf("\nGame tests: %d/%d passed\n", passed, total);
    return passed == total ? 0 : 1;
}

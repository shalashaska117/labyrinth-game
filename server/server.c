#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/protocol.h"
#include "game.h"
#include "log.h"

#define BACKLOG 16
#define DEFAULT_PORT 8080
#define LOG_PATH_DEFAULT "logs/server.log"

typedef enum {
    LOBBY,
    PLAYING,
    FINISHED
} session_state_t;

typedef struct client {
    int fd;
    char nickname[MAX_NICK];
    position_t pos;
    int visible[MAP_HEIGHT][MAP_WIDTH];
    int logged_in;
    int objects_collected;
    int exit_reached;
    time_t exit_time;
    int is_owner;
    int ready;
    struct client *next;
} client_t;

typedef struct {
    char nickname[MAX_NICK];
    int objects_collected;
    int exit_reached;
    time_t exit_time;
} score_entry_t;

static cell_t maze[MAP_HEIGHT][MAP_WIDTH];
static pthread_mutex_t maze_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t score_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
static client_t *client_list = NULL;
static score_entry_t known_scores[MAX_USERS_DISPLAY];
static int known_score_count = 0;
static int server_fd = -1;
static volatile sig_atomic_t running = 1;
static session_state_t session_state = LOBBY;
static int timer_started = 0;

/*
 * Handles process termination signals.
 *
 * Input:
 * - sig: signal number received by the process.
 *
 * Output:
 * - Updates the global running flag.
 *
 * Behavior:
 * - Keeps the handler minimal and signal-safe.
 * - The main accept loop observes running and exits.
 */
static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/*
 * Converts a protocol direction string into the internal movement character.
 *
 * Input:
 * - s: protocol direction string.
 *
 * Output:
 * - Returns w, a, s or d for valid directions.
 * - Returns ? for invalid directions.
 *
 * Behavior:
 * - Accepts only the protocol constants used by the client.
 */
static char dir_to_char(const char *s) {
    if (strcmp(s, DIR_UP) == 0) {
        return 'w';
    }

    if (strcmp(s, DIR_DOWN) == 0) {
        return 's';
    }

    if (strcmp(s, DIR_LEFT) == 0) {
        return 'a';
    }

    if (strcmp(s, DIR_RIGHT) == 0) {
        return 'd';
    }

    return '?';
}

/*
 * Builds a USERS response while the client mutex is already locked.
 *
 * Input:
 * - users_buf: destination buffer.
 * - size: size of the destination buffer.
 *
 * Output:
 * - Writes a complete USERS multiline response into users_buf.
 *
 * Behavior:
 * - Includes connected clients only.
 * - Marks owner and ready clients in the displayed user line.
 * - Terminates the response with END.
 */
static void build_user_list_locked(char *users_buf, size_t size) {
    int count = 0;
    int pos = 0;

    if (users_buf == NULL || size == 0) {
        return;
    }

    for (client_t *tmp = client_list; tmp != NULL; tmp = tmp->next) {
        count++;
    }

    pos += snprintf(users_buf + pos, size - (size_t)pos, "USERS %d\n", count);

    for (client_t *tmp = client_list; tmp != NULL && pos < (int)size; tmp = tmp->next) {
        pos += snprintf(users_buf + pos,
                        size - (size_t)pos,
                        "%s%s%s\n",
                        tmp->nickname[0] ? tmp->nickname : "anonymous",
                        tmp->is_owner ? " [owner]" : "",
                        tmp->ready ? " [ready]" : "");
    }

    if (pos < (int)size) {
        snprintf(users_buf + pos, size - (size_t)pos, "END\n");
    }
}

/*
 * Broadcasts the current user list to all connected clients.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Sends a USERS multiline response to every connected client.
 *
 * Behavior:
 * - Locks the client list while building and sending the response.
 * - Keeps lobby views synchronized after login, register, ready, reset and disconnect.
 */
static void broadcast_user_list(void) {
    char users_buf[BUFFER_SIZE];

    pthread_mutex_lock(&client_mutex);
    build_user_list_locked(users_buf, sizeof(users_buf));

    for (client_t *c = client_list; c != NULL; c = c->next) {
        send_all(c->fd, users_buf, strlen(users_buf));
    }

    pthread_mutex_unlock(&client_mutex);
}

/*
 * Broadcasts one protocol message to all connected clients.
 *
 * Input:
 * - message: newline-terminated protocol message to send.
 *
 * Output:
 * - Attempts to send the message to every connected client.
 *
 * Behavior:
 * - Locks the client list during traversal.
 * - Ignores individual send failures because disconnect cleanup is handled elsewhere.
 */
static void client_list_broadcast(const char *message) {
    pthread_mutex_lock(&client_mutex);

    for (client_t *c = client_list; c != NULL; c = c->next) {
        send_all(c->fd, message, strlen(message));
    }

    pthread_mutex_unlock(&client_mutex);
}

/*
 * Copies one connected client into a score entry.
 *
 * Input:
 * - dst: destination score entry.
 * - src: connected client whose score must be copied.
 *
 * Output:
 * - Updates dst with nickname, object count and exit information.
 *
 * Behavior:
 * - Uses a fallback nickname when the client has not authenticated yet.
 * - Does not retain pointers to the client structure.
 */
static void copy_client_score(score_entry_t *dst, const client_t *src) {
    if (dst == NULL || src == NULL) {
        return;
    }

    snprintf(dst->nickname, sizeof(dst->nickname), "%s", src->nickname[0] ? src->nickname : "anonymous");
    dst->objects_collected = src->objects_collected;
    dst->exit_reached = src->exit_reached;
    dst->exit_time = src->exit_time;
}

/*
 * Saves or updates a score entry in the known-player table.
 *
 * Input:
 * - entry: score entry to store.
 *
 * Output:
 * - Updates known_scores and known_score_count.
 *
 * Behavior:
 * - Replaces an existing entry with the same nickname.
 * - Appends a new entry while there is space.
 * - Keeps disconnected players visible in later RANK responses.
 */
static void remember_score(const score_entry_t *entry) {
    if (entry == NULL || entry->nickname[0] == '\0') {
        return;
    }

    pthread_mutex_lock(&score_mutex);

    for (int i = 0; i < known_score_count; i++) {
        if (strcmp(known_scores[i].nickname, entry->nickname) == 0) {
            known_scores[i] = *entry;
            pthread_mutex_unlock(&score_mutex);
            return;
        }
    }

    if (known_score_count < MAX_USERS_DISPLAY) {
        known_scores[known_score_count++] = *entry;
    }

    pthread_mutex_unlock(&score_mutex);
}

/*
 * Clears the remembered ranking table.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Resets the known score table.
 *
 * Behavior:
 * - Used when a finished session is reset back to the lobby.
 * - Prevents old session results from appearing in the next match.
 */
static void clear_known_scores(void) {
    pthread_mutex_lock(&score_mutex);
    memset(known_scores, 0, sizeof(known_scores));
    known_score_count = 0;
    pthread_mutex_unlock(&score_mutex);
}

/*
 * Resets one client to the lobby gameplay state.
 *
 * Input:
 * - c: client to reset.
 *
 * Output:
 * - Updates client position, readiness, score and visibility.
 *
 * Behavior:
 * - Keeps the socket, nickname, authentication and ownership unchanged.
 * - Clears only per-session gameplay fields.
 */
static void reset_client_for_lobby(client_t *c) {
    if (c == NULL) {
        return;
    }

    c->pos.x = 1;
    c->pos.y = 1;
    c->ready = 0;
    c->objects_collected = 0;
    c->exit_reached = 0;
    c->exit_time = 0;
    memset(c->visible, 0, sizeof(c->visible));
    game_mark_visible(c->visible, c->pos.x, c->pos.y);
}

/*
 * Resets a finished session back to the lobby.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Updates global session state and connected client gameplay state.
 *
 * Behavior:
 * - Sets the session state to LOBBY.
 * - Clears scores, readiness, positions and visibility.
 * - Keeps connected clients and current owner.
 * - Does not generate a new maze immediately; START generates the next maze.
 */
static void reset_session_to_lobby(void) {
    pthread_mutex_lock(&session_mutex);
    session_state = LOBBY;
    timer_started = 0;
    pthread_mutex_unlock(&session_mutex);

    clear_known_scores();

    pthread_mutex_lock(&client_mutex);
    for (client_t *tmp = client_list; tmp != NULL; tmp = tmp->next) {
        reset_client_for_lobby(tmp);
    }
    pthread_mutex_unlock(&client_mutex);
}

/*
 * Adds a newly connected client to the shared client list.
 *
 * Input:
 * - c: client structure allocated by the accept loop.
 *
 * Output:
 * - Modifies the global client list.
 * - Marks the first client as owner.
 *
 * Behavior:
 * - Uses client_mutex because the server is multithreaded.
 * - Assigns ownership before inserting the client in the list.
 */
static void client_list_add(client_t *c) {
    pthread_mutex_lock(&client_mutex);

    c->is_owner = (client_list == NULL) ? 1 : 0;
    c->next = client_list;
    client_list = c;

    pthread_mutex_unlock(&client_mutex);
}

/*
 * Removes a disconnected client from the shared client list.
 *
 * Input:
 * - fd: socket descriptor identifying the client to remove.
 *
 * Output:
 * - Modifies the global client list and frees the removed client.
 * - Saves the final score of the removed client.
 *
 * Behavior:
 * - If the removed client was owner, assigns ownership to the next client.
 * - If no clients remain during PLAYING, marks the session as FINISHED.
 * - Broadcasts an updated user list when clients remain connected.
 */
static void client_list_remove(int fd) {
    client_t *prev = NULL;
    client_t *curr;
    score_entry_t final_score;
    int has_final_score = 0;
    int removed_owner = 0;
    int remaining = 0;

    memset(&final_score, 0, sizeof(final_score));

    pthread_mutex_lock(&client_mutex);

    curr = client_list;
    while (curr != NULL) {
        if (curr->fd == fd) {
            removed_owner = curr->is_owner;
            copy_client_score(&final_score, curr);
            has_final_score = 1;

            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                client_list = curr->next;
            }

            free(curr);
            break;
        }

        prev = curr;
        curr = curr->next;
    }

    if (removed_owner && client_list != NULL) {
        client_list->is_owner = 1;
    }

    for (curr = client_list; curr != NULL; curr = curr->next) {
        remaining++;
    }

    pthread_mutex_unlock(&client_mutex);

    if (has_final_score) {
        remember_score(&final_score);
    }

    if (remaining == 0) {
        pthread_mutex_lock(&session_mutex);
        if (session_state == PLAYING) {
            session_state = FINISHED;
        }
        pthread_mutex_unlock(&session_mutex);
    } else {
        broadcast_user_list();
    }
}

/*
 * Sends an error if a gameplay command is not allowed in the current state.
 *
 * Input:
 * - c: client that issued the command.
 *
 * Output:
 * - Returns 1 if the command must be blocked.
 * - Returns 0 if the command may continue.
 *
 * Behavior:
 * - Blocks gameplay before START.
 * - Blocks gameplay after the session is finished.
 */
static int block_if_not_playing(client_t *c) {
    int blocked = 0;
    session_state_t state;

    pthread_mutex_lock(&session_mutex);
    state = session_state;
    pthread_mutex_unlock(&session_mutex);

    if (state == LOBBY) {
        send_all(c->fd, "ERR game not started\n", strlen("ERR game not started\n"));
        blocked = 1;
    } else if (state == FINISHED) {
        send_all(c->fd, "ERR game finished\n", strlen("ERR game finished\n"));
        blocked = 1;
    }

    return blocked;
}

/*
 * Handles a login command.
 *
 * Input:
 * - c: client that sent the command.
 * - line: full protocol line.
 *
 * Output:
 * - Updates the client authentication state and nickname.
 * - Sends an OK response to the client.
 *
 * Behavior:
 * - Keeps authentication simple, as in the original project.
 * - Uses the supplied nickname if present.
 * - Broadcasts the updated user list after authentication.
 */
static int handle_login(client_t *c, const char *line) {
    char nick[MAX_NICK] = {0};

    sscanf(line + strlen(CMD_LOGIN), "%31s", nick);

    if (nick[0] != '\0') {
        snprintf(c->nickname, sizeof(c->nickname), "%s", nick);
    }

    c->logged_in = 1;
    send_all(c->fd, "OK authenticated\n", strlen("OK authenticated\n"));
    broadcast_user_list();
    return 0;
}

/*
 * Handles a register command.
 *
 * Input:
 * - c: client that sent the command.
 * - line: full protocol line.
 *
 * Output:
 * - Updates the client authentication state and nickname.
 * - Sends an OK response to the client.
 *
 * Behavior:
 * - Keeps registration simple and in-memory.
 * - Does not implement persistent user accounts.
 * - Broadcasts the updated user list after registration.
 */
static int handle_register(client_t *c, const char *line) {
    char nick[MAX_NICK] = {0};

    sscanf(line + strlen(CMD_REGISTER), "%31s", nick);

    if (nick[0] != '\0') {
        snprintf(c->nickname, sizeof(c->nickname), "%s", nick);
    }

    c->logged_in = 1;
    send_all(c->fd, "OK authenticated\n", strlen("OK authenticated\n"));
    broadcast_user_list();
    return 0;
}

/*
 * Handles a READY command in the lobby.
 *
 * Input:
 * - c: client that sent the command.
 *
 * Output:
 * - Marks the client as ready when allowed.
 * - Sends an OK or ERR response.
 *
 * Behavior:
 * - READY is accepted only in LOBBY.
 * - Does not automatically start the match.
 * - Broadcasts the updated user list after readiness changes.
 */
static int handle_ready(client_t *c) {
    session_state_t state;

    pthread_mutex_lock(&session_mutex);
    state = session_state;
    pthread_mutex_unlock(&session_mutex);

    if (state == PLAYING) {
        send_all(c->fd, "ERR game already started\n", strlen("ERR game already started\n"));
        return 0;
    }

    if (state == FINISHED) {
        send_all(c->fd, "ERR game finished\n", strlen("ERR game finished\n"));
        return 0;
    }

    c->ready = 1;
    send_all(c->fd, "OK ready\n", strlen("OK ready\n"));
    broadcast_user_list();
    return 0;
}

/*
 * Handles a RESET command after the session is finished.
 *
 * Input:
 * - c: client that sent RESET.
 *
 * Output:
 * - Sends an error response or broadcasts SESSION LOBBY.
 *
 * Behavior:
 * - Only the current owner can reset.
 * - RESET is accepted only in FINISHED state.
 * - On success, returns all connected clients to LOBBY and refreshes the lobby user list.
 */
static int handle_reset(client_t *c) {
    session_state_t state;

    pthread_mutex_lock(&session_mutex);
    state = session_state;
    pthread_mutex_unlock(&session_mutex);

    if (!c->is_owner) {
        send_all(c->fd, "ERR only owner can reset\n", strlen("ERR only owner can reset\n"));
        return 0;
    }

    if (state != FINISHED) {
        send_all(c->fd, "ERR game not finished\n", strlen("ERR game not finished\n"));
        return 0;
    }

    reset_session_to_lobby();
    client_list_broadcast("SESSION LOBBY\n");
    broadcast_user_list();
    log_msg("SESSION RESET TO LOBBY");
    return 0;
}

/*
 * Runs the session timer thread.
 *
 * Input:
 * - arg: unused thread argument.
 *
 * Output:
 * - Returns NULL when the timer completes.
 * - May update the global session state.
 *
 * Behavior:
 * - Sleeps for SESSION_DURATION seconds.
 * - Ends the session only if it is still PLAYING.
 * - Broadcasts SESSION ENDED to all clients.
 */
static void *session_timer(void *arg) {
    (void)arg;

    sleep(SESSION_DURATION);

    pthread_mutex_lock(&session_mutex);
    if (session_state == PLAYING) {
        session_state = FINISHED;
        timer_started = 0;
        pthread_mutex_unlock(&session_mutex);
        client_list_broadcast("SESSION ENDED\n");
        log_msg("SESSION ENDED by timer");
        return NULL;
    }
    pthread_mutex_unlock(&session_mutex);

    return NULL;
}

/*
 * Starts the gameplay session if the requester is the owner.
 *
 * Input:
 * - c: client that sent START.
 *
 * Output:
 * - Changes the session state to PLAYING when allowed.
 * - Sends or broadcasts a protocol response.
 *
 * Behavior:
 * - Rejects non-owner clients.
 * - Rejects repeated START commands after the game begins.
 * - Initializes the maze and starts one detached timer thread.
 */
static int handle_start(client_t *c) {
    pthread_t timer_thread;

    pthread_mutex_lock(&session_mutex);

    if (session_state == PLAYING) {
        pthread_mutex_unlock(&session_mutex);
        send_all(c->fd, "ERR game already started\n", strlen("ERR game already started\n"));
        return 0;
    }

    if (session_state == FINISHED) {
        pthread_mutex_unlock(&session_mutex);
        send_all(c->fd, "ERR game finished\n", strlen("ERR game finished\n"));
        return 0;
    }

    if (!c->is_owner) {
        pthread_mutex_unlock(&session_mutex);
        send_all(c->fd, "ERR only owner can start\n", strlen("ERR only owner can start\n"));
        return 0;
    }

    pthread_mutex_lock(&maze_mutex);
    game_init(maze);
    pthread_mutex_unlock(&maze_mutex);

    pthread_mutex_lock(&client_mutex);
    for (client_t *tmp = client_list; tmp != NULL; tmp = tmp->next) {
        reset_client_for_lobby(tmp);
    }
    pthread_mutex_unlock(&client_mutex);

    session_state = PLAYING;

    if (!timer_started) {
        timer_started = 1;
        if (pthread_create(&timer_thread, NULL, session_timer, NULL) == 0) {
            pthread_detach(timer_thread);
        } else {
            timer_started = 0;
        }
    }

    pthread_mutex_unlock(&session_mutex);

    client_list_broadcast("SESSION STARTED\n");
    log_msg("SESSION STARTED");
    return 0;
}

/*
 * Handles one MOVE command.
 *
 * Input:
 * - c: client that sent the command.
 * - line: full protocol line.
 *
 * Output:
 * - Sends a local map on successful movement.
 * - Updates object and exit score fields when needed.
 *
 * Behavior:
 * - Blocks movement outside PLAYING state.
 * - Ends the session when a player reaches the exit.
 */
static int handle_move(client_t *c, const char *line) {
    const char *dir_str = line + strlen(CMD_MOVE);
    char d;
    char buf[BUFFER_SIZE];
    int moved;
    int obj;
    int reached_exit;

    while (*dir_str == ' ') {
        dir_str++;
    }

    if (block_if_not_playing(c)) {
        return 0;
    }

    d = dir_to_char(dir_str);
    if (d == '?') {
        send_all(c->fd, "ERR invalid direction\n", strlen("ERR invalid direction\n"));
        return 0;
    }

    pthread_mutex_lock(&maze_mutex);
    moved = game_move(maze, &c->pos, d);

    if (moved == 0) {
        game_mark_visible(c->visible, c->pos.x, c->pos.y);
        obj = game_pickup(maze, c->pos.x, c->pos.y);
        reached_exit = game_is_exit(maze, c->pos.x, c->pos.y);
        game_build_local_view(maze, buf, sizeof(buf), c->pos.x, c->pos.y, c->visible);
    } else {
        obj = 0;
        reached_exit = 0;
    }

    pthread_mutex_unlock(&maze_mutex);

    if (moved < 0) {
        send_all(c->fd, "ERR BLOCKED\n", strlen("ERR BLOCKED\n"));
        return 0;
    }

    send_all(c->fd, buf, strlen(buf));

    if (obj) {
        c->objects_collected++;
        send_all(c->fd, "OK object collected\n", strlen("OK object collected\n"));
    }

    if (reached_exit && !c->exit_reached) {
        c->exit_reached = 1;
        c->exit_time = time(NULL);
        send_all(c->fd, "OK exit found\n", strlen("OK exit found\n"));

        pthread_mutex_lock(&session_mutex);
        if (session_state == PLAYING) {
            session_state = FINISHED;
            timer_started = 0;
            pthread_mutex_unlock(&session_mutex);
            client_list_broadcast("SESSION ENDED\n");
        } else {
            pthread_mutex_unlock(&session_mutex);
        }
    }

    return 0;
}

/*
 * Sends the local map to one client.
 *
 * Input:
 * - c: client requesting the local map.
 *
 * Output:
 * - Sends a MAP LOCAL response to the client.
 *
 * Behavior:
 * - Blocks the command unless the session is PLAYING.
 * - Builds the view using the client's current position and visibility.
 */
static int handle_local(client_t *c) {
    char buf[BUFFER_SIZE];

    if (block_if_not_playing(c)) {
        return 0;
    }

    pthread_mutex_lock(&maze_mutex);
    game_build_local_view(maze, buf, sizeof(buf), c->pos.x, c->pos.y, c->visible);
    pthread_mutex_unlock(&maze_mutex);

    send_all(c->fd, buf, strlen(buf));
    return 0;
}

/*
 * Sends the global masked map to one client.
 *
 * Input:
 * - c: client requesting the global map.
 *
 * Output:
 * - Sends a MAP GLOBAL response to the client.
 *
 * Behavior:
 * - Blocks the command unless the session is PLAYING.
 * - Ensures the current player is rendered with CELL_PLAYER.
 */
static int handle_global(client_t *c) {
    char buf[BUFFER_SIZE];

    if (block_if_not_playing(c)) {
        return 0;
    }

    pthread_mutex_lock(&maze_mutex);
    game_build_global_view(maze, buf, sizeof(buf), c->pos.x, c->pos.y, c->visible);
    pthread_mutex_unlock(&maze_mutex);

    send_all(c->fd, buf, strlen(buf));
    return 0;
}

/*
 * Sends the current user list to one client.
 *
 * Input:
 * - c: client requesting the list.
 *
 * Output:
 * - Sends a USERS multi-line response.
 *
 * Behavior:
 * - Reuses the same format used by automatic lobby broadcasts.
 * - Keeps the response line-based and terminated by END.
 */
static int handle_list(client_t *c) {
    char users_buf[BUFFER_SIZE];

    pthread_mutex_lock(&client_mutex);
    build_user_list_locked(users_buf, sizeof(users_buf));
    pthread_mutex_unlock(&client_mutex);

    send_all(c->fd, users_buf, strlen(users_buf));
    return 0;
}

/*
 * Compares two score entries for ranking order.
 *
 * Input:
 * - a: pointer to the first score entry.
 * - b: pointer to the second score entry.
 *
 * Output:
 * - Returns a negative, zero or positive value for qsort ordering.
 *
 * Behavior:
 * - Ranks exit-reaching players first.
 * - Ranks earlier exit times before later exit times.
 * - Ranks higher object counts before lower object counts.
 */
static int score_compare(const void *a, const void *b) {
    const score_entry_t *sa = (const score_entry_t *)a;
    const score_entry_t *sb = (const score_entry_t *)b;

    if (sa->exit_reached != sb->exit_reached) {
        return sb->exit_reached - sa->exit_reached;
    }

    if (sa->exit_reached && sb->exit_reached) {
        if (sa->exit_time < sb->exit_time) {
            return -1;
        }

        if (sa->exit_time > sb->exit_time) {
            return 1;
        }
    }

    return sb->objects_collected - sa->objects_collected;
}

/*
 * Inserts or replaces one score entry inside a temporary ranking array.
 *
 * Input:
 * - entries: temporary ranking array.
 * - count: pointer to the current number of entries.
 * - entry: score entry to merge into the array.
 *
 * Output:
 * - Updates entries and count.
 *
 * Behavior:
 * - Replaces entries with the same nickname to avoid duplicates.
 * - Appends only when the array still has capacity.
 */
static void merge_score_entry(score_entry_t entries[MAX_USERS_DISPLAY], int *count, const score_entry_t *entry) {
    if (entries == NULL || count == NULL || entry == NULL || entry->nickname[0] == '\0') {
        return;
    }

    for (int i = 0; i < *count; i++) {
        if (strcmp(entries[i].nickname, entry->nickname) == 0) {
            entries[i] = *entry;
            return;
        }
    }

    if (*count < MAX_USERS_DISPLAY) {
        entries[*count] = *entry;
        (*count)++;
    }
}

/*
 * Sends the current ranking to one client.
 *
 * Input:
 * - c: client requesting the ranking.
 *
 * Output:
 * - Sends a RANK multi-line response.
 *
 * Behavior:
 * - Includes connected clients and previously known disconnected clients.
 * - Sorts by exit status, exit time and object count.
 * - Keeps the response easy for the client to parse.
 */
static int handle_rank(client_t *c) {
    score_entry_t entries[MAX_USERS_DISPLAY];
    char buf[BUFFER_SIZE];
    int count = 0;
    int pos = 0;

    pthread_mutex_lock(&score_mutex);
    for (int i = 0; i < known_score_count; i++) {
        merge_score_entry(entries, &count, &known_scores[i]);
    }
    pthread_mutex_unlock(&score_mutex);

    pthread_mutex_lock(&client_mutex);
    for (client_t *tmp = client_list; tmp != NULL; tmp = tmp->next) {
        score_entry_t current;
        copy_client_score(&current, tmp);
        merge_score_entry(entries, &count, &current);
    }
    pthread_mutex_unlock(&client_mutex);

    qsort(entries, (size_t)count, sizeof(entries[0]), score_compare);

    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "RANK %d\n", count);

    for (int i = 0; i < count && pos < (int)sizeof(buf); i++) {
        pos += snprintf(buf + pos,
                        sizeof(buf) - (size_t)pos,
                        "%d %s %d objects%s\n",
                        i + 1,
                        entries[i].nickname,
                        entries[i].objects_collected,
                        entries[i].exit_reached ? " exit" : "");
    }

    snprintf(buf + pos, sizeof(buf) - (size_t)pos, "END\n");
    send_all(c->fd, buf, strlen(buf));
    return 0;
}

/*
 * Sends periodic global maps to all clients during gameplay.
 *
 * Input:
 * - arg: unused thread argument.
 *
 * Output:
 * - Returns NULL when the server stops.
 *
 * Behavior:
 * - Sends maps only while the session state is PLAYING.
 * - Skips lobby and finished states to avoid gameplay output there.
 */
static void *periodic_global(void *arg) {
    (void)arg;

    while (running) {
        session_state_t state;
        sleep(T_INTERVAL);

        pthread_mutex_lock(&session_mutex);
        state = session_state;
        pthread_mutex_unlock(&session_mutex);

        if (state != PLAYING) {
            continue;
        }

        pthread_mutex_lock(&client_mutex);
        for (client_t *c = client_list; c != NULL; c = c->next) {
            char buf[BUFFER_SIZE];

            pthread_mutex_lock(&maze_mutex);
            game_build_global_view(maze, buf, sizeof(buf), c->pos.x, c->pos.y, c->visible);
            pthread_mutex_unlock(&maze_mutex);

            send_all(c->fd, buf, strlen(buf));
        }
        pthread_mutex_unlock(&client_mutex);
    }

    return NULL;
}

/*
 * Handles all commands received from one connected client.
 *
 * Input:
 * - arg: pointer to the client_t structure for the connection.
 *
 * Output:
 * - Returns NULL when the client disconnects.
 * - Closes the client socket and removes the client from the list.
 *
 * Behavior:
 * - Reads line-based protocol messages.
 * - Dispatches each command to the appropriate handler.
 * - Does not expect a nickname line before protocol commands.
 */
static void *handle_client(void *arg) {
    client_t *c = (client_t *)arg;
    char line[MAX_MSG];
    char log_buf[128];

    c->pos.x = 1;
    c->pos.y = 1;
    snprintf(c->nickname, sizeof(c->nickname), "guest%d", c->fd);
    game_mark_visible(c->visible, c->pos.x, c->pos.y);

    snprintf(log_buf, sizeof(log_buf), "CONNECTED: %s", c->nickname);
    log_msg(log_buf);

    send_all(c->fd,
             c->is_owner ? "OK welcome owner\n" : "OK welcome\n",
             strlen(c->is_owner ? "OK welcome owner\n" : "OK welcome\n"));

    broadcast_user_list();

    while (running) {
        if (recv_line(c->fd, line, sizeof(line)) < 0) {
            break;
        }

        if (strcmp(line, CMD_QUIT) == 0) {
            send_all(c->fd, "OK bye\n", strlen("OK bye\n"));
            break;
        }

        if (strncmp(line, CMD_LOGIN, strlen(CMD_LOGIN)) == 0) {
            handle_login(c, line);
        } else if (strncmp(line, CMD_REGISTER, strlen(CMD_REGISTER)) == 0) {
            handle_register(c, line);
        } else if (strncmp(line, CMD_MOVE, strlen(CMD_MOVE)) == 0) {
            handle_move(c, line);
        } else if (strcmp(line, CMD_LOCAL) == 0) {
            handle_local(c);
        } else if (strcmp(line, CMD_GLOBAL) == 0) {
            handle_global(c);
        } else if (strcmp(line, CMD_LIST) == 0) {
            handle_list(c);
        } else if (strcmp(line, CMD_READY) == 0) {
            handle_ready(c);
        } else if (strcmp(line, CMD_START) == 0) {
            handle_start(c);
        } else if (strcmp(line, CMD_RESET) == 0) {
            handle_reset(c);
        } else if (strcmp(line, CMD_RANK) == 0) {
            handle_rank(c);
        } else {
            send_all(c->fd, "ERR unknown command\n", strlen("ERR unknown command\n"));
        }
    }

    close(c->fd);
    snprintf(log_buf, sizeof(log_buf), "DISCONNECTED: %s", c->nickname);
    log_msg(log_buf);
    client_list_remove(c->fd);
    return NULL;
}

/*
 * Server process entry point.
 *
 * Input:
 * - argc: number of command-line arguments.
 * - argv: optional port and optional log path.
 *
 * Output:
 * - Returns 0 on normal shutdown and 1 on startup error.
 *
 * Behavior:
 * - Creates a TCP listening socket.
 * - Accepts clients and creates one detached thread per client.
 * - Starts in LOBBY and waits for the owner to issue START.
 */
int main(int argc, char *argv[]) {
    const char *log_path = LOG_PATH_DEFAULT;
    int port = DEFAULT_PORT;
    int opt = 1;
    struct sockaddr_in addr;
    pthread_t periodic_thread;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    if (argc > 2) {
        log_path = argv[2];
    }

    if (log_init(log_path) < 0) {
        fprintf(stderr, "Failed to open log: %s\n", log_path);
        return 1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        log_msg("FATAL: socket() failed");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        log_msg("FATAL: bind() failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        log_msg("FATAL: listen() failed");
        close(server_fd);
        return 1;
    }

    if (pthread_create(&periodic_thread, NULL, periodic_global, NULL) == 0) {
        pthread_detach(periodic_thread);
    }

    log_msg("SERVER STARTED in LOBBY");

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        client_t *c;
        pthread_t th;

        if (client_fd < 0) {
            if (running) {
                perror("accept");
            }
            break;
        }

        c = calloc(1, sizeof(client_t));
        if (c == NULL) {
            close(client_fd);
            continue;
        }

        c->fd = client_fd;
        client_list_add(c);

        if (pthread_create(&th, NULL, handle_client, c) == 0) {
            pthread_detach(th);
        } else {
            close(client_fd);
            client_list_remove(client_fd);
        }
    }

    close(server_fd);
    log_msg("SERVER SHUTDOWN");
    log_close();
    return 0;
}
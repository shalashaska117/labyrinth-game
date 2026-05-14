#ifndef STATE_H
#define STATE_H

#include "../common/constants.h"
#include "../common/protocol.h"

typedef enum {
    CLIENT_LOBBY,
    CLIENT_PLAYING,
    CLIENT_FINISHED
} client_session_state_t;

typedef enum {
    MOVEMENT,
    COMMAND
} input_mode_t;

typedef struct {
    char nickname[MAX_NICK];
    int logged_in;
    int pending_auth;
    char pending_nickname[MAX_NICK];
    input_mode_t mode;
    client_session_state_t session_state;
    int is_owner;
    int player_count;
    char player_list[MAX_USERS_DISPLAY][MAX_MSG];
    int player_objects[MAX_USERS_DISPLAY];
    int show_global;
    int local_rows;
    int local_cols;
    int global_rows;
    int global_cols;
    char local_map[(VIEW_RADIUS * 2 + 1) * (VIEW_RADIUS * 2 + 1)];
    char global_map[MAP_WIDTH * MAP_HEIGHT];
    char input_buffer[MAX_MSG];
    int input_pos;
    char status_message[MAX_MSG];
    int rank_count;
    char rank_lines[MAX_USERS_DISPLAY][MAX_MSG];
    int time_remaining;
    int exit_reached;
} ClientState;

void init_client_state(ClientState *state);
void set_pending_auth(ClientState *state, const char *nickname);
void confirm_auth(ClientState *state);
void clear_pending_auth(ClientState *state);
void state_set_status(ClientState *state, const char *message);

#endif

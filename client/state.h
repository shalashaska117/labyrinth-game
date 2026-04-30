#ifndef STATE_H
#define STATE_H

#include "../common/protocol.h"

/*
 * Represents the current authentication state of the client.
 *
 * nickname: nickname currently associated with the client.
 * logged_in: 1 if the client is authenticated, 0 otherwise.
 * pending_auth: 1 if the client is waiting for a login/register response.
 * pending_nickname: nickname used in the last login/register request.
 */
typedef struct {
    char nickname[MAX_NICK];
    int logged_in;
    int pending_auth;
    char pending_nickname[MAX_NICK];
} ClientState;

/*
 * Initializes a ClientState object.
 */
void init_client_state(ClientState *state);

/*
 * Marks the client as waiting for authentication.
 */
void set_pending_auth(ClientState *state, const char *nickname);

/*
 * Marks authentication as completed successfully.
 */
void confirm_auth(ClientState *state);

/*
 * Clears pending authentication after an error.
 */
void clear_pending_auth(ClientState *state);

#endif

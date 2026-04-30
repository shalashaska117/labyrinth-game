#ifndef RESPONSE_H
#define RESPONSE_H

#include "state.h"

/*
 * Handles a complete server response starting from the first received line.
 *
 * sock: socket connected to the server.
 * first_line: first line already received from the server.
 * state: current client state, updated after authentication responses.
 *
 * Returns:
 *   0 if the message is handled correctly.
 *  -1 if a protocol error occurs or the server disconnects.
 */
int handle_server_message(int sock, const char *first_line, ClientState *state);

#endif

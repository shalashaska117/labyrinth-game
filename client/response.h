#ifndef RESPONSE_H
#define RESPONSE_H

#include "state.h"

int handle_server_message(int sock, const char *first_line, ClientState *state);

#endif

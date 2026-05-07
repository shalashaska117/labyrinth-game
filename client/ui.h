#ifndef UI_H
#define UI_H

#include "state.h"
#include "../common/protocol.h"

void print_help(void);
void clear_screen(void);
void print_map(const char *title, const char *map, int rows, int cols);
void print_users(char users[][MAX_NICK], int count);
int terminal_set_raw(void);
void terminal_restore(void);
void draw_screen(const ClientState *state);

#endif

#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/*
 * Converts a user-friendly command into a protocol command.
 *
 * input: command typed by the user.
 * output: buffer where the protocol command is written.
 * size: maximum size of the output buffer.
 *
 * Returns:
 *   1 if the command must be sent to the server.
 *   0 if the command was handled locally by the client.
 *  -1 if the command is invalid.
 */
int build_protocol_command(const char *input, char *output, size_t size);

#endif

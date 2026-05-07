#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

#define MAX_MSG 1024
#define MAX_NICK 32
#define MAX_PASSWORD 64

#define CMD_REGISTER "REGISTER"
#define CMD_LOGIN    "LOGIN"
#define CMD_MOVE     "MOVE"
#define CMD_LIST     "LIST"
#define CMD_LOCAL    "LOCAL"
#define CMD_GLOBAL   "GLOBAL"
#define CMD_QUIT     "QUIT"
#define CMD_START    "START"
#define CMD_READY    "READY"
#define CMD_RANK     "RANK"
#define CMD_RESET   "RESET"

#define RESP_OK      "OK"
#define RESP_ERR     "ERR"
#define RESP_MAP     "MAP"
#define RESP_USERS   "USERS"
#define RESP_END     "END"
#define RESP_RANK    "RANK"
#define RESP_SESSION "SESSION"

#define MAP_LOCAL    "LOCAL"
#define MAP_GLOBAL   "GLOBAL"

#define DIR_UP    "UP"
#define DIR_DOWN  "DOWN"
#define DIR_LEFT  "LEFT"
#define DIR_RIGHT "RIGHT"

/*
 * Sends the entire buffer through the socket.
 *
 * Input:
 * - sock: socket file descriptor.
 * - buffer: data to send.
 * - length: number of bytes to send.
 *
 * Output:
 * - Returns 0 on success and -1 on error.
 *
 * Behavior:
 * - Repeats send() until all bytes are transmitted.
 * - Stops immediately if the socket reports an error or closes.
 */
int send_all(int sock, const char *buffer, size_t length);

/*
 * Receives a single protocol line from the socket.
 *
 * Input:
 * - sock: socket file descriptor.
 * - buffer: destination buffer.
 * - size: maximum buffer size.
 *
 * Output:
 * - Returns 0 on success and -1 on error or disconnection.
 * - Writes a null-terminated line without the final newline into buffer.
 *
 * Behavior:
 * - Reads one character at a time until a newline is found.
 * - Preserves bounded writes and always terminates the string.
 */
int recv_line(int sock, char *buffer, size_t size);

#endif

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

#define RESP_OK      "OK"
#define RESP_ERR     "ERR"
#define RESP_MAP     "MAP"
#define RESP_USERS   "USERS"
#define RESP_END     "END"

#define MAP_LOCAL    "LOCAL"
#define MAP_GLOBAL   "GLOBAL"

#define DIR_UP    "UP"
#define DIR_DOWN  "DOWN"
#define DIR_LEFT  "LEFT"
#define DIR_RIGHT "RIGHT"

/*
 * Sends the entire buffer through the socket.
 *
 * sock: socket file descriptor.
 * buffer: data to send.
 * length: number of bytes to send.
 *
 * The function handles partial sends.
 *
 * Returns 0 on success, -1 on error.
 */
int send_all(int sock, const char *buffer, size_t length);

/*
 * Receives a single line from the socket.
 *
 * sock: socket file descriptor.
 * buffer: destination buffer.
 * size: maximum buffer size.
 *
 * The function stops when it finds a newline character.
 * The resulting string is always null-terminated.
 *
 * Returns 0 on success, -1 on error or disconnection.
 */
int recv_line(int sock, char *buffer, size_t size);

#endif

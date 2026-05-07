#include "protocol.h"

#include <unistd.h>
#include <sys/socket.h>

/*
 * Sends all bytes contained in a buffer.
 *
 * Input:
 * - sock: socket file descriptor.
 * - buffer: source buffer to transmit.
 * - length: number of bytes to transmit.
 *
 * Output:
 * - Returns 0 if all bytes are sent.
 * - Returns -1 if send() fails or the connection closes.
 *
 * Behavior:
 * - Handles partial send() operations by advancing through the buffer.
 * - Does not add terminators or protocol newlines by itself.
 */
int send_all(int sock, const char *buffer, size_t length) {
    size_t total = 0;

    while (total < length) {
        ssize_t sent = send(sock, buffer + total, length - total, 0);

        if (sent <= 0) {
            return -1;
        }

        total += (size_t)sent;
    }

    return 0;
}

/*
 * Receives one newline-terminated protocol line.
 *
 * Input:
 * - sock: socket file descriptor.
 * - buffer: destination buffer.
 * - size: destination buffer size.
 *
 * Output:
 * - Returns 0 when a line is read.
 * - Returns -1 on recv() error, closed connection or invalid buffer size.
 * - Stores the received line without the newline character.
 *
 * Behavior:
 * - Reads one byte at a time to keep the line-based protocol simple.
 * - Truncates safely if the line exceeds the buffer capacity.
 */
int recv_line(int sock, char *buffer, size_t size) {
    size_t i = 0;
    char c;

    if (buffer == NULL || size == 0) {
        return -1;
    }

    while (i < size - 1) {
        ssize_t received = recv(sock, &c, 1, 0);

        if (received <= 0) {
            return -1;
        }

        if (c == '\n') {
            break;
        }

        buffer[i++] = c;
    }

    buffer[i] = '\0';
    return 0;
}

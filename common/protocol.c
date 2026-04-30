#include "protocol.h"

#include <unistd.h>
#include <sys/socket.h>

/*
 * Sends all bytes contained in buffer.
 *
 * sock: socket descriptor.
 * buffer: data to send.
 * length: total number of bytes to send.
 *
 * send() may transmit fewer bytes than requested. For this reason,
 * this function keeps calling send() until the whole buffer has been
 * transmitted or an error occurs.
 *
 * Returns 0 if all bytes are sent, -1 otherwise.
 */
int send_all(int sock, const char *buffer, size_t length) {
    size_t total = 0;
    ssize_t sent;

    while (total < length) {
        sent = send(sock, buffer + total, length - total, 0);

        if (sent <= 0) {
            return -1;
        }

        total += sent;
    }

    return 0;
}

/*
 * Receives a line from the socket.
 *
 * sock: socket descriptor.
 * buffer: destination buffer.
 * size: maximum size of the destination buffer.
 *
 * The function reads one character at a time and stops when it receives
 * a newline, when the buffer is full, or when an error occurs.
 *
 * The resulting string is always null-terminated.
 *
 * Returns 0 on success, -1 on error or closed connection.
 */
int recv_line(int sock, char *buffer, size_t size) {
    size_t i = 0;
    char c;
    ssize_t received;

    while (i < size - 1) {
        received = recv(sock, &c, 1, 0);

        if (received <= 0) {
            return -1;
        }

        if (c == '\n') {
            break;
        }

        buffer[i] = c;
        i++;
    }

    buffer[i] = '\0';
    return 0;
}

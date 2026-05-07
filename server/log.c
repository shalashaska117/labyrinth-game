#include "log.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int log_fd = -1;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Opens the server log file.
 *
 * Input:
 * - path: filesystem path of the log file.
 *
 * Output:
 * - Returns 0 on success and -1 on error.
 * - Updates the internal log file descriptor.
 *
 * Behavior:
 * - Opens the file in append mode.
 * - Creates the file if it does not already exist.
 */
int log_init(const char *path) {
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (log_fd < 0) {
        perror("log_init: open");
        return -1;
    }

    return 0;
}

/*
 * Writes a timestamped message to the server log.
 *
 * Input:
 * - message: message text to append to the log.
 *
 * Output:
 * - Returns 0 on success and -1 on error.
 *
 * Behavior:
 * - Prepends a local timestamp to every log line.
 * - Uses a mutex because several client threads can write concurrently.
 */
int log_msg(const char *message) {
    time_t now;
    struct tm *tm_info;
    char timestamp[64];
    char buf[1088];
    int n;
    int write_res;

    if (log_fd < 0 || message == NULL) {
        return -1;
    }

    now = time(NULL);
    tm_info = localtime(&now);

    if (tm_info == NULL) {
        return -1;
    }

    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);
    n = snprintf(buf, sizeof(buf), "%s %s\n", timestamp, message);

    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return -1;
    }

    pthread_mutex_lock(&log_mutex);
    write_res = (write(log_fd, buf, (size_t)n) == (ssize_t)n) ? 0 : -1;
    pthread_mutex_unlock(&log_mutex);

    return write_res;
}

/*
 * Closes the server log file.
 *
 * Input:
 * - None.
 *
 * Output:
 * - Returns 0 on success and -1 if the log was not open.
 * - Resets the internal log file descriptor.
 *
 * Behavior:
 * - Protects close() with the same mutex used for writes.
 */
int log_close(void) {
    if (log_fd < 0) {
        return -1;
    }

    pthread_mutex_lock(&log_mutex);
    close(log_fd);
    log_fd = -1;
    pthread_mutex_unlock(&log_mutex);

    return 0;
}

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

static int log_fd = -1;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int log_init(const char *path) {
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("log_init: open");
        return -1;
    }
    return 0;
}

int log_msg(const char *message) {
    if (log_fd < 0) return -1;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (!tm) return -1;

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm);

    char buf[1088]; //64 bytes (timestamp) + 1024 bytes (buffer)
    int n = snprintf(buf, sizeof(buf), "%s %s\n", timestamp, message);
    if (n < 0 || (size_t)n >= sizeof(buf)) 
			return -1;

    pthread_mutex_lock(&log_mutex);
    int write_res = (write(log_fd, buf, (size_t)n) == (ssize_t)n) ? 0 : -1;
    pthread_mutex_unlock(&log_mutex);

    return write_res;
}

int log_close(void) {
    if (log_fd < 0) return -1;

    pthread_mutex_lock(&log_mutex);
    
	  close(log_fd);
    log_fd = -1;
    
		pthread_mutex_unlock(&log_mutex);

    return 0;
}

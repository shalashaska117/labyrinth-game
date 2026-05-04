#ifndef LOG_H
#define LOG_H

int log_init(const char *path);
int log_msg(const char *message);
int log_close(void);

#endif

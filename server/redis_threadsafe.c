#include "redis_threadsafe.h"

#include <pthread.h>
#include <stdarg.h>

/*
 * Mutex protecting the shared Redis context used by the server.
 */
static pthread_mutex_t redis_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Executes a Redis command while holding the Redis mutex.
 *
 * The caller is responsible for freeing the returned redisReply with
 * freeReplyObject().
 */
void *redis_command_locked(redisContext *context, const char *format, ...) {
    va_list args;
    void *reply;

    pthread_mutex_lock(&redis_mutex);

    va_start(args, format);
    reply = redisvCommand(context, format, args);
    va_end(args);

    pthread_mutex_unlock(&redis_mutex);

    return reply;
}

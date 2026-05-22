#ifndef REDIS_THREADSAFE_H
#define REDIS_THREADSAFE_H

#include <hiredis/hiredis.h>

/*
 * Executes one Redis command while holding a global mutex.
 *
 * The server uses one shared redisContext for all client threads. This wrapper
 * serializes access to that context so login and registration commands cannot
 * use Hiredis concurrently.
 */
void *redis_command_locked(redisContext *context, const char *format, ...);

#endif

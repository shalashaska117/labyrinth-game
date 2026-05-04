#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "../common/protocol.h"
#include "../common/constants.h"
#include "log.h"
#include "game.h"

#define BACKLOG 16
#define DEFAULT_PORT 8080
#define LOG_PATH_DEFAULT "../logs/server.log"

static cell_t maze[MAP_HEIGHT][MAP_WIDTH];
static pthread_mutex_t maze_mutex = PTHREAD_MUTEX_INITIALIZER;
static int server_fd = -1;
static volatile sig_atomic_t running = 1;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

static char dir_to_char(const char *s) {
    if (strcmp(s, DIR_UP)    == 0) return 'w';
    if (strcmp(s, DIR_DOWN)  == 0) return 's';
    if (strcmp(s, DIR_LEFT)  == 0) return 'a';
    if (strcmp(s, DIR_RIGHT) == 0) return 'd';
    return '?';
}

typedef struct client {
    int fd;
    char nickname[MAX_NICK];
    position_t pos;
    int visible[MAP_HEIGHT][MAP_WIDTH];
    int logged_in;
    struct client *next;
} client_t;

static client_t *client_list = NULL;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

static void client_list_add(client_t *c) {
    pthread_mutex_lock(&client_mutex);
    c->next = client_list;
    client_list = c;
    pthread_mutex_unlock(&client_mutex);
}

static void client_list_remove(int fd) {
    pthread_mutex_lock(&client_mutex);
    client_t *prev = NULL;
    client_t *curr = client_list;
    while (curr) {
        if (curr->fd == fd) {
            if (prev)
                prev->next = curr->next;
            else
                client_list = curr->next;
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&client_mutex);
}

// ====================== HANDLE REGION ==================================

static int handle_login(client_t *c, const char *line) {
    c->logged_in = 1;
    char nick[MAX_NICK] = {0};
    sscanf(line + strlen(CMD_LOGIN), "%s", nick);
    if (nick[0]) {
        snprintf(c->nickname, sizeof(c->nickname), "%s", nick);
        char buf[64];
        snprintf(buf, sizeof(buf), "LOGIN: %s", nick);
        log_msg(buf);
    }
    send_all(c->fd, "OK authenticated\n", strlen("OK authenticated\n"));
    return 0;
}

static int handle_register(client_t *c, const char *line) {
    c->logged_in = 1;
    char nick[MAX_NICK] = {0};
    sscanf(line + strlen(CMD_REGISTER), "%s", nick);
    if (nick[0]) {
        snprintf(c->nickname, sizeof(c->nickname), "%s", nick);
        char buf[64];
        snprintf(buf, sizeof(buf), "REGISTER: %s", nick);
        log_msg(buf);
    }
    send_all(c->fd, "OK authenticated\n", strlen("OK authenticated\n"));
    return 0;
}

static int handle_move(client_t *c, const char *line) {
    const char *dir_str = line + strlen(CMD_MOVE) + 1;
    char d = dir_to_char(dir_str);
    if (d == '?') {
        send_all(c->fd, "ERR invalid direction\n", strlen("ERR invalid direction\n"));
        return 0;
    }

    char buf[BUFFER_SIZE];
    pthread_mutex_lock(&maze_mutex);
    int moved = game_move(maze, &c->pos, d);
    if (moved == 0) {
        game_mark_visible(c->visible, c->pos.x, c->pos.y);
        game_build_local_view(maze, buf, sizeof(buf), c->pos.x, c->pos.y, c->visible);
    }
    int obj = game_pickup(maze, c->pos.x, c->pos.y);
    int exit_reached = game_is_exit(maze, c->pos.x, c->pos.y);
    pthread_mutex_unlock(&maze_mutex);

    if (moved < 0) {
        send_all(c->fd, "ERR BLOCKED\n", strlen("ERR BLOCKED\n"));
        return 0;
    }

    send_all(c->fd, buf, strlen(buf));

    if (obj) {
        send_all(c->fd, "OK object collected\n", strlen("OK object collected\n"));
        char log_buf[96];
        snprintf(log_buf, sizeof(log_buf), "OBJECT: %s at (%d,%d)",
                 c->nickname, c->pos.x, c->pos.y);
        log_msg(log_buf);
    }

    if (exit_reached) {
        send_all(c->fd, "OK exit found\n", strlen("OK exit found\n"));
        char log_buf[96];
        snprintf(log_buf, sizeof(log_buf), "EXIT: %s at (%d,%d)",
                 c->nickname, c->pos.x, c->pos.y);
        log_msg(log_buf);
    }

    return 0;
}

static int handle_local(client_t *c) {
    char buf[BUFFER_SIZE];
    pthread_mutex_lock(&maze_mutex);
    game_build_local_view(maze, buf, sizeof(buf), c->pos.x, c->pos.y, c->visible);
    pthread_mutex_unlock(&maze_mutex);
    send_all(c->fd, buf, strlen(buf));
    return 0;
}

static int handle_global(client_t *c) {
    char buf[BUFFER_SIZE];
    pthread_mutex_lock(&maze_mutex);
    game_build_global_view(maze, buf, sizeof(buf), c->visible);
    pthread_mutex_unlock(&maze_mutex);
    send_all(c->fd, buf, strlen(buf));
    return 0;
}

static int handle_list(client_t *c) {
    char users_buf[BUFFER_SIZE];
    int count = 0;
    int pos = 0;

    pthread_mutex_lock(&client_mutex);
    for (client_t *tmp = client_list; tmp; tmp = tmp->next)
        count++;
    pos += snprintf(users_buf + pos, sizeof(users_buf) - pos, "USERS %d\n", count);
    for (client_t *tmp = client_list; tmp; tmp = tmp->next)
        pos += snprintf(users_buf + pos, sizeof(users_buf) - pos, "%s\n", tmp->nickname);
    pthread_mutex_unlock(&client_mutex);

    snprintf(users_buf + pos, sizeof(users_buf) - pos, "END\n");
    send_all(c->fd, users_buf, strlen(users_buf));
    return 0;
}

static void *handle_client(void *arg) {
    client_t *c = (client_t *)arg;
    char line[MAX_MSG];

    c->pos.x = 1;
    c->pos.y = 1;

    if (recv_line(c->fd, c->nickname, sizeof(c->nickname)) < 0) {
        close(c->fd);
        free(c);
        return NULL;
    }

    char log_buf[64];
    snprintf(log_buf, sizeof(log_buf), "CONNECTED: %s", c->nickname);
    log_msg(log_buf);

    pthread_mutex_lock(&maze_mutex);
    game_mark_visible(c->visible, c->pos.x, c->pos.y);
    pthread_mutex_unlock(&maze_mutex);

    send_all(c->fd, "OK welcome\n", strlen("OK welcome\n"));

    while (running) {
        if (recv_line(c->fd, line, sizeof(line)) < 0)
            break;
				
			  // Handle Quit
        if (strcmp(line, CMD_QUIT) == 0) {
            send_all(c->fd, "OK bye\n", strlen("OK bye\n"));
            break;
        }

			  // Handle login
        if (strncmp(line, CMD_LOGIN, strlen(CMD_LOGIN)) == 0) {
            handle_login(c, line);
            continue;
        }
				// Handle Register
        if (strncmp(line, CMD_REGISTER, strlen(CMD_REGISTER)) == 0) {
            handle_register(c, line);
            continue;
        }
				
				// Handle Move
        if (strncmp(line, CMD_MOVE, strlen(CMD_MOVE)) == 0) {
            handle_move(c, line);
            continue;
        }
				
				// Handle Local Map
        if (strcmp(line, CMD_LOCAL) == 0) {
            handle_local(c);
            continue;
        }
				
				// Handle Global Map
        if (strcmp(line, CMD_GLOBAL) == 0) {
            handle_global(c);
            continue;
        }
				
				// Handle Client list
        if (strcmp(line, CMD_LIST) == 0) {
            handle_list(c);
            continue;
        }

        send_all(c->fd, "OK command received\n", strlen("OK command received\n"));
    }

    close(c->fd);
    snprintf(log_buf, sizeof(log_buf), "DISCONNECTED: %s", c->nickname);
    log_msg(log_buf);
    client_list_remove(c->fd);
    return NULL;
}

// ====================== END HANDLE REGION ==================================

static void *periodic_global(void *arg) {
    (void)arg;
    char buf[BUFFER_SIZE];

    while (running) {
        sleep(T_INTERVAL);
        pthread_mutex_lock(&client_mutex);

        for (client_t *c = client_list; c; c = c->next) {
            pthread_mutex_lock(&maze_mutex);
            game_build_global_view(maze, buf, sizeof(buf), c->visible);
            pthread_mutex_unlock(&maze_mutex);
            send_all(c->fd, buf, strlen(buf));
        }

        pthread_mutex_unlock(&client_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    const char *log_path = LOG_PATH_DEFAULT;
    int port = DEFAULT_PORT;

    if (argc > 1) port = atoi(argv[1]); //atoi is cleaner, but the error is silent.
    if (argc > 2) log_path = argv[2];

    if (log_init(log_path) < 0) {
        fprintf(stderr, "Failed to open log: %s\n", log_path);
        return 1;
    }

    game_init(maze);
    log_msg("SERVER STARTED — maze generated");

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        log_msg("FATAL: socket() failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { htonl(INADDR_ANY) },
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        log_msg("FATAL: bind() failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        log_msg("FATAL: listen() failed");
        close(server_fd);
        return 1;
    }

    char port_buf[64];
    snprintf(port_buf, sizeof(port_buf), "LISTENING on port %d", port);
    log_msg(port_buf);

    pthread_t periodic_thread;
    // pthread_create(&periodic_thread, NULL, periodic_global, NULL);
    // pthread_detach(periodic_thread);

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0) {
            if (running) perror("accept");
            break;
        }

        client_t *c = calloc(1, sizeof(client_t));
        if (!c) {
            close(client_fd);
            continue;
        }
        c->fd = client_fd;
        client_list_add(c);

        pthread_t th;
        pthread_create(&th, NULL, handle_client, c);
        pthread_detach(th);
    }

    close(server_fd);
    log_msg("SERVER SHUTDOWN");
    log_close();
    return 0;
}

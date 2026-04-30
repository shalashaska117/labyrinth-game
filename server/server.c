#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../common/protocol.h"

/*
 * Minimal placeholder server used only to keep the project compilable until
 * Persona A replaces it with the complete concurrent server.
 */
int main(int argc, char *argv[]) {
    int server_fd;
    int client_fd;
    int port;
    int opt;
    struct sockaddr_in addr;
    char line[MAX_MSG];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    send_all(client_fd, "OK placeholder server ready\n", strlen("OK placeholder server ready\n"));

    while (recv_line(client_fd, line, sizeof(line)) == 0) {
        if (strcmp(line, CMD_QUIT) == 0) {
            send_all(client_fd, "OK bye\n", strlen("OK bye\n"));
            break;
        }

        if (strncmp(line, CMD_LOGIN, strlen(CMD_LOGIN)) == 0 ||
            strncmp(line, CMD_REGISTER, strlen(CMD_REGISTER)) == 0) {
            send_all(client_fd, "OK authenticated\n", strlen("OK authenticated\n"));
        } else if (strcmp(line, CMD_LOCAL) == 0) {
            send_all(client_fd, "MAP LOCAL 3 3\n###\n#P#\n###\nEND\n", strlen("MAP LOCAL 3 3\n###\n#P#\n###\nEND\n"));
        } else if (strcmp(line, CMD_GLOBAL) == 0) {
            send_all(client_fd, "MAP GLOBAL 3 3\n???\n?P?\n???\nEND\n", strlen("MAP GLOBAL 3 3\n???\n?P?\n???\nEND\n"));
        } else if (strcmp(line, CMD_LIST) == 0) {
            send_all(client_fd, "USERS 1\nplaceholder\nEND\n", strlen("USERS 1\nplaceholder\nEND\n"));
        } else {
            send_all(client_fd, "OK command received\n", strlen("OK command received\n"));
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}

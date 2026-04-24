CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -pthread
LDFLAGS = -pthread
BUILD_DIR = bin

.PHONY: all clean server client

all: server client

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

server: $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/server src/server.c $(LDFLAGS)

client: $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/client src/client.c $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all debug release client server test test-command test-state test-response-tcp coverage lcov lcov-report valgrind-test-command valgrind-test-state valgrind-test-response valgrind-client valgrind-server run-client run-server clean

CC=gcc

CFLAGS=-Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200112L
DEBUG_FLAGS=-g -O0
RELEASE_FLAGS=-O3
COVERAGE_FLAGS=-g -O0 --coverage

PTHREAD_FLAGS=-pthread

COMMON=common/protocol.c

CLIENT_SRC=client/client.c client/ui.c client/command.c client/response.c client/state.c $(COMMON)
SERVER_SRC=server/server.c server/game.c server/log.c $(COMMON)

CLIENT_BIN=client_app
SERVER_BIN=server_app

TEST_COMMAND_SRC=tests/test_command_parser.c client/command.c client/ui.c $(COMMON)
TEST_COMMAND_BIN=test_command_parser

TEST_STATE_SRC=tests/test_state.c client/state.c
TEST_STATE_BIN=test_state

TEST_RESPONSE_TCP_SRC=tests/test_response_tcp.c client/response.c client/ui.c client/state.c $(COMMON)
TEST_RESPONSE_TCP_BIN=test_response_tcp

all: debug

debug: CFLAGS += $(DEBUG_FLAGS)
debug: client server

release: CFLAGS += $(RELEASE_FLAGS)
release: client server

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC) $(PTHREAD_FLAGS)

test-command:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_COMMAND_BIN) $(TEST_COMMAND_SRC)
	./$(TEST_COMMAND_BIN)

test-state:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_STATE_BIN) $(TEST_STATE_SRC)
	./$(TEST_STATE_BIN)

test-response-tcp:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_RESPONSE_TCP_BIN) $(TEST_RESPONSE_TCP_SRC)
	./$(TEST_RESPONSE_TCP_BIN)

test: test-command test-state test-response-tcp

coverage: clean
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(SERVER_BIN) $(SERVER_SRC) $(PTHREAD_FLAGS)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_COMMAND_BIN) $(TEST_COMMAND_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_STATE_BIN) $(TEST_STATE_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_RESPONSE_TCP_BIN) $(TEST_RESPONSE_TCP_SRC)

lcov: coverage
	@echo "Run tests first with: ./$(TEST_COMMAND_BIN) && ./$(TEST_STATE_BIN) && ./$(TEST_RESPONSE_TCP_BIN)"
	@echo "Then execute: make lcov-report"

lcov-report:
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated in coverage_report/index.html"

valgrind-test-command: test-command
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_COMMAND_BIN)

valgrind-test-state: test-state
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_STATE_BIN)

valgrind-test-response: test-response-tcp
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_RESPONSE_TCP_BIN)

valgrind-client: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(CLIENT_BIN) 127.0.0.1 5000

valgrind-server: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(SERVER_BIN) 5000

run-client: client
	./$(CLIENT_BIN) 127.0.0.1 5000

run-server: server
	./$(SERVER_BIN) 5000

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -f $(TEST_COMMAND_BIN) $(TEST_STATE_BIN) $(TEST_RESPONSE_TCP_BIN)
	rm -f *.o
	find . -name "*.gcda" -delete
	find . -name "*.gcno" -delete
	rm -f *.gcov coverage.info
	rm -rf coverage_report
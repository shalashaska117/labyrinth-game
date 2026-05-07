.PHONY: help all debug release client server test test-command test-state test-response-tcp coverage lcov lcov-report valgrind-test-command valgrind-test-state valgrind-test-response valgrind-client valgrind-server run-client run-server clean clean-logs

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

help:
	@echo "Available make targets:"
	@echo ""
	@echo "  make help                    Show this help message."
	@echo "  make all                     Build the project in debug mode."
	@echo "  make debug                   Build client and server with debug flags."
	@echo "  make release                 Build client and server with optimization flags."
	@echo "  make client                  Build only the client executable."
	@echo "  make server                  Build only the server executable."
	@echo ""
	@echo "Tests:"
	@echo "  make test                    Build and run all unit/integration tests."
	@echo "  make test-command            Build and run command parser tests."
	@echo "  make test-state              Build and run client state tests."
	@echo "  make test-response-tcp       Build and run TCP response parser tests."
	@echo ""
	@echo "Coverage:"
	@echo "  make coverage                Build project and tests with coverage flags."
	@echo "  make lcov                    Build coverage binaries and print next steps."
	@echo "  make lcov-report             Generate LCOV HTML report after running tests."
	@echo ""
	@echo "Valgrind:"
	@echo "  make valgrind-test-command   Run command parser tests under Valgrind."
	@echo "  make valgrind-test-state     Run client state tests under Valgrind."
	@echo "  make valgrind-test-response  Run TCP response parser tests under Valgrind."
	@echo "  make valgrind-client         Run the client under Valgrind."
	@echo "  make valgrind-server         Run the server under Valgrind."
	@echo ""
	@echo "Run:"
	@echo "  make run-client              Build and run the client on 127.0.0.1:5000."
	@echo "  make run-server              Build and run the server on port 5000."
	@echo ""
	@echo "Clean:"
	@echo "  make clean                   Remove binaries, test executables and coverage files."
	@echo "  make clean-logs              Remove log files while preserving logs/.gitkeep."

all: debug

# =========================
# BUILDS
# =========================

debug: CFLAGS += $(DEBUG_FLAGS)
debug: client server

release: CFLAGS += $(RELEASE_FLAGS)
release: client server

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC) $(PTHREAD_FLAGS)

# =========================
# TESTS
# =========================

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

# =========================
# COVERAGE
# =========================

coverage: clean
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(SERVER_BIN) $(SERVER_SRC) $(PTHREAD_FLAGS)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_COMMAND_BIN) $(TEST_COMMAND_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_STATE_BIN) $(TEST_STATE_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_RESPONSE_TCP_BIN) $(TEST_RESPONSE_TCP_SRC)

lcov: coverage
	@echo "Coverage binaries were built successfully."
	@echo ""
	@echo "Run the tests with:"
	@echo "  ./$(TEST_COMMAND_BIN) && ./$(TEST_STATE_BIN) && ./$(TEST_RESPONSE_TCP_BIN)"
	@echo ""
	@echo "Then generate the report with:"
	@echo "  make lcov-report"

lcov-report:
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated in coverage_report/index.html"

# =========================
# VALGRIND
# =========================

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

# =========================
# RUN
# =========================

run-client: client
	./$(CLIENT_BIN) 127.0.0.1 5000

run-server: server
	./$(SERVER_BIN) 5000

# =========================
# CLEAN
# =========================

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -f $(TEST_COMMAND_BIN) $(TEST_STATE_BIN) $(TEST_RESPONSE_TCP_BIN)
	rm -f *.o
	find . -name "*.gcda" -delete
	find . -name "*.gcno" -delete
	rm -f *.gcov coverage.info
	rm -rf coverage_report

clean-logs:
	mkdir -p logs
	find logs -type f ! -name ".gitkeep" -delete 2>/dev/null || true
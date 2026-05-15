.PHONY: help all build debug release client server test test-command test-command-extended test-state test-state-extended test-response-tcp test-protocol-extended test-game coverage coverage-run lcov lcov-report lcov-open open-coverage valgrind-test valgrind-test-command valgrind-test-command-extended valgrind-test-state valgrind-test-state-extended valgrind-test-response valgrind-test-protocol-extended valgrind-test-game valgrind-client valgrind-server run-client run-server clean clean-logs

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

TEST_COMMAND_EXT_SRC=tests/test_command_extended.c client/command.c client/ui.c $(COMMON)
TEST_COMMAND_EXT_BIN=test_command_extended

TEST_STATE_SRC=tests/test_state.c client/state.c
TEST_STATE_BIN=test_state

TEST_STATE_EXT_SRC=tests/test_state_extended.c client/state.c
TEST_STATE_EXT_BIN=test_state_extended

TEST_RESPONSE_TCP_SRC=tests/test_response_tcp.c client/response.c client/ui.c client/state.c $(COMMON)
TEST_RESPONSE_TCP_BIN=test_response_tcp

TEST_PROTOCOL_EXT_SRC=tests/test_protocol_extended.c $(COMMON)
TEST_PROTOCOL_EXT_BIN=test_protocol_extended

TEST_GAME_SRC=tests/test_game.c
TEST_GAME_BIN=test_game

help:
	@echo "Available make targets:"
	@echo ""
	@echo "  make help                         Show this help message."
	@echo "  make all                          Build the project in debug mode."
	@echo "  make build                        Build both client and server without running."
	@echo "  make debug                        Build client and server with debug flags."
	@echo "  make release                      Build client and server with optimization flags."
	@echo "  make client                       Build only the client executable."
	@echo "  make server                       Build only the server executable."
	@echo ""
	@echo "Tests:"
	@echo "  make test                         Build and run all automated tests."
	@echo "  make test-command                 Build and run base command parser tests."
	@echo "  make test-command-extended        Build and run extended command parser tests."
	@echo "  make test-state                   Build and run base client state tests."
	@echo "  make test-state-extended          Build and run extended client state tests."
	@echo "  make test-response-tcp            Build and run TCP response parser tests."
	@echo "  make test-protocol-extended       Build and run extended protocol helper tests."
	@echo "  make test-game                    Build and run server game logic tests."
	@echo ""
	@echo "Coverage:"
	@echo "  make coverage                     Build project and tests with coverage flags."
	@echo "  make coverage-run                 Build coverage binaries and run coverage tests."
	@echo "  make lcov                         Build, run coverage tests and generate the LCOV HTML report."
	@echo "  make lcov-report                  Generate LCOV HTML report from existing coverage data."
	@echo "  make lcov-open                    Build, run coverage tests, generate and open the LCOV HTML report."
	@echo "  make open-coverage                Open an already generated LCOV HTML report."
	@echo ""
	@echo "Valgrind:"
	@echo "  make valgrind-test                Run all automated tests under Valgrind."
	@echo "  make valgrind-test-command        Run base command parser tests under Valgrind."
	@echo "  make valgrind-test-command-extended Run extended command parser tests under Valgrind."
	@echo "  make valgrind-test-state          Run base client state tests under Valgrind."
	@echo "  make valgrind-test-state-extended Run extended client state tests under Valgrind."
	@echo "  make valgrind-test-response       Run TCP response parser tests under Valgrind."
	@echo "  make valgrind-test-protocol-extended Run extended protocol helper tests under Valgrind."
	@echo "  make valgrind-test-game           Run server game logic tests under Valgrind."
	@echo "  make valgrind-client              Run the client under Valgrind."
	@echo "  make valgrind-server              Run the server under Valgrind."
	@echo ""
	@echo "Run:"
	@echo "  make run-client                   Build and run the client on 127.0.0.1:8080."
	@echo "  make run-server                   Build and run the server on port 8080."
	@echo ""
	@echo "Clean:"
	@echo "  make clean                        Remove binaries, test executables and coverage files."
	@echo "  make clean-logs                   Remove log files while preserving logs/.gitkeep."

all: debug

build: debug

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

test-command-extended:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_COMMAND_EXT_BIN) $(TEST_COMMAND_EXT_SRC)
	./$(TEST_COMMAND_EXT_BIN)

test-state:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_STATE_BIN) $(TEST_STATE_SRC)
	./$(TEST_STATE_BIN)

test-state-extended:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_STATE_EXT_BIN) $(TEST_STATE_EXT_SRC)
	./$(TEST_STATE_EXT_BIN)

test-response-tcp:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_RESPONSE_TCP_BIN) $(TEST_RESPONSE_TCP_SRC)
	./$(TEST_RESPONSE_TCP_BIN)

test-protocol-extended:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_PROTOCOL_EXT_BIN) $(TEST_PROTOCOL_EXT_SRC)
	./$(TEST_PROTOCOL_EXT_BIN)

test-game:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TEST_GAME_BIN) $(TEST_GAME_SRC)
	./$(TEST_GAME_BIN)

test: test-command test-command-extended test-state test-state-extended test-response-tcp test-protocol-extended test-game

# =========================
# COVERAGE
# =========================

coverage: clean
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(SERVER_BIN) $(SERVER_SRC) $(PTHREAD_FLAGS)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_COMMAND_BIN) $(TEST_COMMAND_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_COMMAND_EXT_BIN) $(TEST_COMMAND_EXT_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_STATE_BIN) $(TEST_STATE_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_STATE_EXT_BIN) $(TEST_STATE_EXT_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_RESPONSE_TCP_BIN) $(TEST_RESPONSE_TCP_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_PROTOCOL_EXT_BIN) $(TEST_PROTOCOL_EXT_SRC)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -o $(TEST_GAME_BIN) $(TEST_GAME_SRC)

coverage-run: coverage
	./$(TEST_COMMAND_BIN)
	./$(TEST_COMMAND_EXT_BIN)
	./$(TEST_STATE_BIN)
	./$(TEST_STATE_EXT_BIN)
	./$(TEST_RESPONSE_TCP_BIN)
	./$(TEST_PROTOCOL_EXT_BIN)
	./$(TEST_GAME_BIN)

lcov: coverage-run
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' '*/tests/*' --ignore-errors unused --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated in coverage_report/index.html"

lcov-report:
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' '*/tests/*' --ignore-errors unused --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated in coverage_report/index.html"

lcov-open: lcov
	explorer.exe "$$(wslpath -w coverage_report/index.html)" || true

open-coverage:
	explorer.exe "$$(wslpath -w coverage_report/index.html)" || true

# =========================
# VALGRIND
# =========================

valgrind-test: valgrind-test-command valgrind-test-command-extended valgrind-test-state valgrind-test-state-extended valgrind-test-response valgrind-test-protocol-extended valgrind-test-game

valgrind-test-command: test-command
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_COMMAND_BIN)

valgrind-test-command-extended: test-command-extended
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_COMMAND_EXT_BIN)

valgrind-test-state: test-state
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_STATE_BIN)

valgrind-test-state-extended: test-state-extended
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_STATE_EXT_BIN)

valgrind-test-response: test-response-tcp
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_RESPONSE_TCP_BIN)

valgrind-test-protocol-extended: test-protocol-extended
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_PROTOCOL_EXT_BIN)

valgrind-test-game: test-game
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_GAME_BIN)

valgrind-client: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(CLIENT_BIN) 127.0.0.1 8080

valgrind-server: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(SERVER_BIN) 8080

# =========================
# RUN
# =========================

run-client: client
	./$(CLIENT_BIN) 127.0.0.1 8080

run-server: server
	./$(SERVER_BIN) 8080

# =========================
# CLEAN
# =========================

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -f $(TEST_COMMAND_BIN) $(TEST_COMMAND_EXT_BIN)
	rm -f $(TEST_STATE_BIN) $(TEST_STATE_EXT_BIN)
	rm -f $(TEST_RESPONSE_TCP_BIN) $(TEST_PROTOCOL_EXT_BIN) $(TEST_GAME_BIN)
	rm -f *.o
	find . -name "*.gcda" -delete
	find . -name "*.gcno" -delete
	rm -f *.gcov coverage.info
	rm -rf coverage_report

clean-logs:
	mkdir -p logs
	find logs -type f ! -name ".gitkeep" -delete 2>/dev/null || true
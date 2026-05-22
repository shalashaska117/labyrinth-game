# Labyrinth Game

A C client-server labyrinth game for Unix/Linux systems, developed as an Operating Systems laboratory project.

The project implements a multiplayer TCP labyrinth in which users connect to a server, wait in a lobby, start a timed session, explore a generated maze, collect objects, reach the exit, and view a final ranking.

## Documentation

- [View the project documentation](docs/documentation.pdf)
- [Download the project documentation](docs/documentation.pdf)
- [View the communication protocol](docs/protocol.md)

## Overview

The application is divided into two main programs:

- `server_app`: manages clients, lobby state, game sessions, maze generation, player state, scoring and rankings.
- `client_app`: connects to the server, sends commands, receives protocol responses and renders a terminal interface.

The communication between client and server is based on a simple line-oriented text protocol over TCP sockets.

## Main Features

- TCP client-server architecture.
- Multiplayer lobby with session owner.
- Ready/start workflow.
- Timed gameplay session.
- Local and global map views.
- Per-player visibility.
- Object collection.
- Exit detection.
- Final ranking.
- Reset from finished state back to lobby.
- Persistent terminal UI.
- Command mode and movement mode.
- Automated tests.
- LCOV coverage report generation.
- Valgrind targets.
- Docker support.
- Redis/Hiredis-based authentication backend.

## Technologies and Constraints

The project is written in C and uses Unix/POSIX primitives.

Main primitives and tools used:

- `socket`, `bind`, `listen`, `accept`, `connect`
- `send`, `recv`, `read`, `write`, `close`
- `getaddrinfo`
- `select`
- `pthread`
- mutexes
- `termios` on the client side only
- Redis and Hiredis for the authentication backend
- Makefile
- Valgrind
- LCOV
- Docker / Docker Compose

## Ports

The project uses two different network ports:

```text
8080 -> labyrinth game server
6379 -> Redis authentication backend
```

The client connects to the game server on port `8080`.

Redis is used only by the server for the authentication backend. The client never connects directly to Redis.

## Linux Dependencies

This project is intended to build and run on Linux or WSL.

On Debian, Ubuntu or WSL, install the required development tools with:

```bash
sudo apt update
sudo apt install build-essential libhiredis-dev redis-server valgrind lcov
```

Before running the server natively, make sure Redis is running:

```bash
sudo service redis-server start
redis-cli ping
```

The expected Redis response is:

```text
PONG
```

The project is configured for Linux-style Hiredis linking through `-lhiredis`.

No macOS/Homebrew path is required by default.

## Repository Structure

```text
.
├── client/              Client-side logic, UI, command translation and response parsing
├── common/              Shared protocol helpers and constants
├── server/              Server, game logic, session management and logging
├── tests/               Automated tests
├── docs/                Project documentation and protocol reference
├── scripts/             Utility scripts
├── logs/                Runtime logs, ignored except for .gitkeep
├── Makefile
├── Dockerfile
└── docker-compose.yml
```

## Build

Build client and server in debug mode:

```bash
make debug
```

Build optimized binaries:

```bash
make release
```

Show all available Makefile targets:

```bash
make help
```

## Run Natively on Linux/WSL

Start Redis:

```bash
sudo service redis-server start
redis-cli ping
```

Build the project:

```bash
make clean
make debug
```

Start the server:

```bash
./server_app 8080
```

Or use the Makefile target:

```bash
make run-server
```

Start a client from another terminal:

```bash
./client_app 127.0.0.1 8080
```

Or use the Makefile target:

```bash
make run-client
```

Multiple clients can be started in separate terminals.

## Basic Usage

After connecting, use command mode to register or log in:

```text
register pietro secret
login pietro secret
```

Useful commands:

```text
ready
start
users
local
global
rank
scoreboard
reset
quit
```

During gameplay, press `TAB` to switch between command mode and movement mode.

Movement mode keys:

```text
W  move up
A  move left
S  move down
D  move right
G  toggle/request global map
L  request local map
Q  quit
```

## Game Lifecycle

The server manages three states:

```text
LOBBY
PLAYING
FINISHED
```

In the lobby, players can connect, register, log in and mark themselves ready.

The first connected client becomes the session owner. Only the owner can start the session.

During gameplay, players explore the maze, collect objects and try to reach the exit.

When the session finishes, the ranking remains available. The owner can reset the session back to the lobby.

## Ranking

The ranking is ordered by:

1. players who reached the exit;
2. score among players who reached the exit, based on collected objects and elapsed time;
3. highest number of collected objects among players who did not reach the exit.

This means that reaching the exit has priority over simply collecting more objects.

The current ranking response format is documented in [`docs/protocol.md`](docs/protocol.md).

## Testing

Run all automated tests:

```bash
make test
```

Run all automated tests under Valgrind:

```bash
make valgrind-test
```

Generate the LCOV coverage report:

```bash
make lcov
```

On WSL, generate and open the report:

```bash
make lcov-open
```

The generated report is stored in:

```text
coverage_report/index.html
```

## Cleaning

Remove generated binaries, test executables and coverage files:

```bash
make clean
```

Remove runtime logs while preserving `logs/.gitkeep`:

```bash
make clean-logs
```

If the server cannot open `logs/server.log` because of Windows/WSL permissions, recreate the log directory:

```bash
rm -rf logs
mkdir logs
touch logs/.gitkeep
```

Alternatively, run the server with a temporary log path:

```bash
./server_app 8080 /tmp/labyrinth-server.log
```

## Docker

The project includes Docker support through `Dockerfile` and `docker-compose.yml`.

Docker Compose starts:

- a Redis service, used by the authentication backend;
- the labyrinth server, listening on port `8080`;
- an optional client service.

The recommended workflow is to run the server with Docker Compose and run the clients from a native Linux/WSL terminal, because the client uses `termios` for interactive terminal input.

## Docker Networking Notes

The Docker setup uses this idea:

```text
Host / WSL client   -> 127.0.0.1 8080
Docker client       -> redis 8080
Redis backend       -> 6379
```

This happens because the server shares the Redis service network namespace in Docker Compose.

Therefore:

- from WSL/Linux or from the host, connect to the game server with `127.0.0.1 8080`;
- from another Docker container, connect to the game server with `redis 8080`;
- Redis itself remains on port `6379`.

## Recommended Docker Workflow: Server in Docker, Clients from WSL/Linux

First, stop old containers:

```bash
docker compose down --remove-orphans
```

Optionally clean the log directory:

```bash
rm -rf logs
mkdir logs
touch logs/.gitkeep
```

Start Redis and the server with Docker Compose:

```bash
docker compose up --build server
```

The server listens on port `8080`, exposed on the host as:

```text
127.0.0.1:8080
```

Then, from another Linux/WSL terminal, build and run the native client:

```bash
make debug
./client_app 127.0.0.1 8080
```

To start additional players, open other Linux/WSL terminals and run:

```bash
./client_app 127.0.0.1 8080
```

This is the most reliable Docker workflow for interactive play.

## Fully Docker-Based Execution

The client can also be started inside Docker Compose.

First, start Redis and the server:

```bash
docker compose up --build server
```

Then, from another terminal, start a client container:

```bash
docker compose run --rm --no-deps client redis 8080
```

Important: when the client runs inside Docker, use `redis` as the host, not `127.0.0.1`.

Inside the client container, `127.0.0.1` means the client container itself, not the server. Because the server shares the Redis service network namespace, the game server is reachable from Docker clients at:

```text
redis:8080
```

From the host system or from a native WSL/Linux terminal, use instead:

```text
127.0.0.1:8080
```

Summary:

```text
Native WSL/Linux client -> 127.0.0.1 8080
Docker client           -> redis 8080
Redis service           -> 6379
```

## Generic Docker Compose Start

Running:

```bash
docker compose up --build
```

builds the project and starts the default services, namely Redis and the labyrinth server.

The Docker client is assigned to the optional `client` profile, because it is interactive and normally requires explicit host and port arguments.

Recommended:

```bash
docker compose up --build server
```

Then, for a Docker client:

```bash
docker compose run --rm --no-deps client redis 8080
```

Or, for a native WSL/Linux client:

```bash
./client_app 127.0.0.1 8080
```

## Stop Docker Services

To stop and remove the running containers:

```bash
docker compose down
```

To also remove orphan containers:

```bash
docker compose down --remove-orphans
```

## Docker Logs

The server container writes logs to the project `logs/` directory through this volume:

```yaml
volumes:
  - ./logs:/app/logs
```

Therefore, a log written inside the container as:

```text
/app/logs/server.log
```

is available on the host as:

```text
logs/server.log
```

When the server is launched natively from WSL/Linux from the project root, it also writes to the same `logs/` directory. Actual log files are ignored by Git, while `logs/.gitkeep` preserves the directory in the repository.

## Quick Native Test

```bash
sudo service redis-server start
redis-cli ping

make clean
make debug
make test

make run-server
```

In another terminal:

```bash
make run-client
```

## Quick Docker Test

Terminal 1:

```bash
docker compose down --remove-orphans
rm -rf logs
mkdir logs
touch logs/.gitkeep
docker compose up --build server
```

Terminal 2, native WSL/Linux client:

```bash
make debug
./client_app 127.0.0.1 8080
```

Terminal 3, Docker client:

```bash
docker compose run --rm --no-deps client redis 8080
```

## Notes

This repository contains the source code and technical assets of the project.

The complete academic documentation is provided separately in `docs/documentation.pdf`.

The protocol-level documentation is available in `docs/protocol.md`.

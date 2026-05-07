# Labyrinth Game

A C client-server labyrinth game for Unix/Linux systems, developed as an Operating Systems laboratory project.

The project implements a multiplayer TCP labyrinth in which users connect to a server, wait in a lobby, start a timed session, explore a generated maze, collect objects, reach the exit, and view a final ranking.

## Documentation

- [View the project documentation](docs/documentation.pdf)
- [Download the project documentation](docs/documentation.pdf)

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
- Makefile
- Valgrind
- LCOV
- Docker / Docker Compose

## Repository Structure

```text
.
├── client/              Client-side logic, UI, command translation and response parsing
├── common/              Shared protocol helpers and constants
├── server/              Server, game logic, session management and logging
├── tests/               Automated tests
├── docs/                Project documentation
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

## Run

Start the server:

```bash
./server_app 5000
```

Start a client:

```bash
./client_app 127.0.0.1 5000
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
2. earliest exit time;
3. highest number of collected objects.

This means that reaching the exit has priority over simply collecting more objects.

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

## Docker

Build and run with Docker Compose:

```bash
docker compose up --build
```

## Notes

This repository contains the source code and technical assets of the project.  
The complete academic documentation is provided separately in `docs/documentation.pdf`.

# Labyrinth Multiplayer Game (Client-Server in C)

## Introduction

This project implements a multiplayer labyrinth exploration game using a client-server architecture in C on a Unix/Linux environment.

Multiple users can connect to a central server and explore a randomly generated 2D maze. Each player can move inside the map, collect objects, and try to reach an exit. The server maintains the full game state, while each client only sees a partial view based on discovered cells.

Communication between client and server is done through TCP sockets using a custom text-based protocol.

The project is developed following a modular approach and uses only standard C libraries and system calls covered in the course.

---

## Features

- TCP client-server communication
- Support for multiple simultaneous clients
- Text-based protocol for commands and responses
- Local map view (based on discovered cells)
- Global masked map view
- User authentication (login/register)
- Command parsing and response handling
- Terminal-based user interface
- Automated tests for client logic
- Memory checks with Valgrind
- Code coverage support (lcov)
- Docker support for easy execution

---

## Project Structure

```
.
├── client/
├── server/
├── common/
├── tests/
├── scripts/
├── logs/
├── Dockerfile
├── docker-compose.yml
├── Makefile
└── README.md
```

---

## Requirements

- Linux or WSL
- gcc
- make
- valgrind (optional)
- lcov (optional)
- Docker and Docker Compose

---

## Build Instructions

```
make debug
make release
make clean
```

---

## Running with Docker

```
docker compose up server
docker compose run --rm client server 5000
```

---

## Client Commands

```
login <username> <password>
register <username> <password>
w s a d
local
global
users
help
quit
```

---

## Protocol Example

```
MAP LOCAL 3 3
###
#P#
###
END
```

---

## Testing

```
make test
```

---

## Valgrind

```
make valgrind-test-command
make valgrind-test-state
make valgrind-test-response
```

---

## Coverage

```
make coverage
./test_command_parser
./test_state
./test_response_tcp
make lcov-report
```

---

## Notes

- Server does not read from stdin
- Protocol is line-based with END delimiter
- Client supports hostname "server" in Docker

---

## Conclusion

This project demonstrates a complete client-server system in C using sockets, select(), and a modular design.

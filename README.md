# Labyrinth game
Client-server multiplayer maze game written in C, using TCP sockets and pthreads.

## Project structure

```
├── src/               # C source files (server.c, client.c)
├── TracciaA-LSO-2025.pdf
├── roadmap_notebooklm.pdf
├── Makefile           # Build with gcc on Linux
├── Dockerfile         # Multi-stage: builder, server, client targets
├── docker-compose.yml # Orchestrated dev environment
└── .gitignore
```

## Quick start with Docker (cross-platform)

### Build and run the server

```bash
docker compose up server
```

The server listens on port `8080` by default.

### Run a client (in another terminal)

```bash
docker compose run --rm client
```

This connects to the `server` container on port `8080`.

### Build locally (Linux/macOS)

```bash
make all
./bin/server
./bin/client <server-addr> <port>
```

## Requirements

The server requires a UNIX-like environment. On Windows, use Docker or WSL.

## Protocol

See `TracciaA-LSO-2025.pdf` for the application-level protocol specification.

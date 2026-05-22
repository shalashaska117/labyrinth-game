# Client-Server Communication Protocol

This document describes the line-based text protocol used by the `labyrinth-game` project.
Every command and every response line ends with `\n`.

The client sends textual commands to the server through a TCP socket.
The server replies with either single-line messages or multiline responses terminated by `END`.

---

## 1. General Rules

- Every command sent by the client ends with a newline.
- Every response sent by the server ends with a newline.
- Every multiline response terminates with:

```text
END
```

- Protocol commands are uppercase.
- The client may accept lowercase commands and slash-prefixed commands, then translate them to uppercase protocol commands.
- Some server messages may be asynchronous, especially `USERS`, `SESSION`, `MAP GLOBAL` and `TIME` messages.

Example:

```text
start
```

is sent as:

```text
START
```

---

## 2. Supported Commands

### REGISTER

Registers a new user and authenticates the current client.

```text
REGISTER <nickname> <password>
```

Possible responses:

```text
OK authenticated
ERR usage: REGISTER <nickname> <password>
ERR registration unavailable
ERR nickname cannot start with guest
ERR nickname already registered
ERR nickname already in use
```

---

### LOGIN

Authenticates an existing user.

```text
LOGIN <nickname> <password>
```

Possible responses:

```text
OK authenticated
ERR usage: LOGIN <nickname> <password>
ERR authentication unavailable
ERR user not registered
ERR wrong password
ERR nickname already in use
```

---

### LIST

Requests the list of currently connected users.

```text
LIST
```

Client aliases:

```text
users
list
/users
/list
```

Response format:

```text
USERS <n>
<nickname> [owner] [ready]
...
END
```

Example:

```text
USERS 3
pietro [owner]
mario [ready]
guest7
END
```

Notes:

- `[owner]` marks the current session owner.
- `[ready]` marks a client that issued `READY` in the lobby.
- Unauthenticated clients are shown with generated guest names.

---

### READY

Marks the client as ready while the session is still in the lobby.

```text
READY
```

Possible responses:

```text
OK ready
ERR game already started
ERR game finished
```

Readiness is tracked and shown in the `USERS` response, but it does not automatically start the game.

---

### START

Starts the game session.

```text
START
```

Rules:

- only the current session owner can execute `START`;
- the first connected client becomes the initial owner;
- if the owner disconnects, ownership is reassigned;
- `START` is accepted only in `LOBBY`;
- all non-owner clients must be authenticated and ready;
- the current implementation requires at least one non-owner client to be ready before the owner can start;
- on success, the server generates the maze, assigns spawn positions, starts the timer and enters `PLAYING`.

Possible responses:

```text
SESSION STARTED
ERR only owner can start
ERR not all players are ready
ERR game already started
ERR game finished
```

---

### MOVE

Moves the player in one direction.

```text
MOVE <direction>
```

Valid directions:

```text
UP
DOWN
LEFT
RIGHT
```

Client movement aliases:

```text
w -> MOVE UP
a -> MOVE LEFT
s -> MOVE DOWN
d -> MOVE RIGHT
```

Successful movement response:

```text
MAP LOCAL <rows> <cols>
<row_1>
<row_2>
...
END
```

Possible error responses:

```text
ERR game not started
ERR game finished
ERR invalid direction
ERR BLOCKED
```

Additional messages may follow a successful movement:

```text
OK object collected
OK exit found
SESSION ENDED
```

---

### LOCAL

Requests the local map around the player.

```text
LOCAL
```

Valid only during `PLAYING`.

Response format:

```text
MAP LOCAL <rows> <cols>
<row_1>
<row_2>
...
END
```

Possible error responses:

```text
ERR game not started
ERR game finished
```

---

### GLOBAL

Requests the masked global map.

```text
GLOBAL
```

Valid only during `PLAYING`.

Response format:

```text
MAP GLOBAL <rows> <cols>
<row_1>
<row_2>
...
END
```

Map symbols:

```text
# wall
  empty cell
* object
E exit
P current player
O other active player
? hidden cell
```

Possible error responses:

```text
ERR game not started
ERR game finished
```

---

### RANK

Requests the current or final ranking.

```text
RANK
```

Client aliases:

```text
rank
scoreboard
/rank
/scoreboard
```

Response format:

```text
RANK <n>
<position>. <nickname> - <objects> objects - exit in <seconds>s - Score: <score>
<position>. <nickname> - <objects> objects - did not exit
...
END
```

Example:

```text
RANK 3
1. pietro - 5 objects - exit in 42s - Score: 458
2. mario - 3 objects - did not exit
3. lisa - 0 objects - did not exit
END
```

Sorting rules:

1. players who reached the exit are ranked before players who did not;
2. exiting players are scored using collected objects and elapsed time;
3. non-exiting players are ranked by collected objects.

---

### RESET

Returns a finished session back to the lobby.

```text
RESET
```

Client aliases:

```text
reset
/reset
```

Rules:

- only the current owner can execute `RESET`;
- `RESET` is accepted only in `FINISHED`;
- on success, the server clears remembered scores, readiness, positions and visibility;
- connected clients remain connected;
- the next `START` generates a new maze.

Possible responses:

```text
SESSION LOBBY
ERR only owner can reset
ERR game not finished
```

---

### QUIT

Disconnects the client from the server.

```text
QUIT
```

Response:

```text
OK bye
```

---

## 3. Supported Server Responses

### OK

```text
OK <message>
```

Examples:

```text
OK welcome owner
OK welcome
OK authenticated
OK ready
OK object collected
OK exit found
OK bye
```

---

### ERR

```text
ERR <message>
```

Examples:

```text
ERR game not started
ERR game already started
ERR game finished
ERR only owner can start
ERR not all players are ready
ERR only owner can reset
ERR game not finished
ERR invalid direction
ERR BLOCKED
ERR unknown command
```

---

### SESSION

```text
SESSION STARTED
SESSION ENDED
SESSION LOBBY
```

Meaning:

- `SESSION STARTED`: the game session has started;
- `SESSION ENDED`: the game session has ended;
- `SESSION LOBBY`: a finished session has been reset back to the lobby.

---

### TIME

Contains the remaining session time in seconds.

```text
TIME <seconds>
```

Example:

```text
TIME 245
```

Sent periodically during `PLAYING`.

---

### USERS

```text
USERS <n>
<nickname> [owner] [ready]
...
END
```

---

### MAP LOCAL

```text
MAP LOCAL <rows> <cols>
<row_1>
<row_2>
...
END
```

---

### MAP GLOBAL

```text
MAP GLOBAL <rows> <cols>
<row_1>
<row_2>
...
END
```

---

### RANK

```text
RANK <n>
<position>. <nickname> - <objects> objects - exit in <seconds>s - Score: <score>
<position>. <nickname> - <objects> objects - did not exit
...
END
```

---

## 4. Server Session States

```c
typedef enum {
    LOBBY,
    PLAYING,
    FINISHED
} session_state_t;
```

### LOBBY

Allowed commands:

```text
REGISTER
LOGIN
LIST
READY
START
RANK
QUIT
```

Blocked commands:

```text
MOVE
LOCAL
GLOBAL
RESET
```

Typical responses:

```text
ERR game not started
ERR game not finished
```

### PLAYING

Allowed commands:

```text
MOVE
LOCAL
GLOBAL
LIST
RANK
QUIT
```

Rejected commands:

```text
START -> ERR game already started
READY -> ERR game already started
RESET -> ERR game not finished
```

During this state:

- movement is enabled;
- the session timer is active;
- the server may send periodic `MAP GLOBAL` updates;
- the server may send periodic `TIME <seconds>` updates.

### FINISHED

Allowed commands:

```text
RANK
LIST
RESET
QUIT
```

Blocked commands:

```text
MOVE
LOCAL
GLOBAL
START
READY
```

Typical response:

```text
ERR game finished
```

---

## 5. Client States and Input Modes

```c
typedef enum {
    CLIENT_LOBBY,
    CLIENT_PLAYING,
    CLIENT_FINISHED
} client_session_state_t;
```

```c
typedef enum {
    MOVEMENT,
    COMMAND
} input_mode_t;
```

### MOVEMENT

```text
w/W -> MOVE UP
a/A -> MOVE LEFT
s/S -> MOVE DOWN
d/D -> MOVE RIGHT
g/G -> toggle global overlay and request GLOBAL
l/L -> request LOCAL
Q   -> QUIT
TAB -> switch to COMMAND mode
```

### COMMAND

```text
register <nickname> <credential>
login <nickname> <credential>
users
list
ready
start
reset
rank
scoreboard
local
global
quit
```

Slash-prefixed variants are also accepted.

---

## 6. Main Constants

Commands:

```c
#define CMD_REGISTER "REGISTER"
#define CMD_LOGIN    "LOGIN"
#define CMD_MOVE     "MOVE"
#define CMD_LIST     "LIST"
#define CMD_LOCAL    "LOCAL"
#define CMD_GLOBAL   "GLOBAL"
#define CMD_QUIT     "QUIT"
#define CMD_START    "START"
#define CMD_READY    "READY"
#define CMD_RANK     "RANK"
#define CMD_RESET    "RESET"
```

Responses:

```c
#define RESP_OK      "OK"
#define RESP_ERR     "ERR"
#define RESP_MAP     "MAP"
#define RESP_USERS   "USERS"
#define RESP_END     "END"
#define RESP_RANK    "RANK"
#define RESP_SESSION "SESSION"
```

Map types:

```c
#define MAP_LOCAL    "LOCAL"
#define MAP_GLOBAL   "GLOBAL"
```

Directions:

```c
#define DIR_UP       "UP"
#define DIR_DOWN     "DOWN"
#define DIR_LEFT     "LEFT"
#define DIR_RIGHT    "RIGHT"
```

Timers:

```c
#define SESSION_DURATION 300
#define T_INTERVAL 5
```

---

## 7. Typical Execution Sequence

Owner client:

```text
REGISTER pietro <credential>
LIST
RANK
START
LOCAL
GLOBAL
MOVE UP
RANK
QUIT
```

Second client:

```text
REGISTER mario <credential>
READY
LIST
RANK
QUIT
```

Expected response to `START` from a non-owner client:

```text
ERR only owner can start
```

Expected response to `START` from the owner when not all non-owner clients are ready:

```text
ERR not all players are ready
```

Finished session reset sequence:

```text
RANK
RESET
LIST
START
```

---

## 8. Implementation Notes

- The server owns the game logic.
- The client owns rendering, terminal input and local display state.
- `termios` is used only on the client side.
- Communication remains textual and human-readable.
- Every multiline response must end with `END`.
- `REGISTER` and `LOGIN` depend on the authentication backend in the current implementation.
- The server may broadcast `USERS`, `SESSION`, `MAP GLOBAL` and `TIME` messages without a direct one-to-one request from the client.

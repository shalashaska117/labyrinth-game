# Client-Server Communication Protocol

This document describes the text-based protocol used by the `labyrinth-game` project.
The protocol is line-based: every command and every response line ends with `\n`.

The client sends textual commands to the server through a TCP socket.
The server replies with either single-line messages or multiline responses terminated by `END`.

---

## 1. General Rules

- Every command sent by the client ends with a newline.
- Every response sent by the server ends with a newline.
- Every multiline response must terminate with the following line:

```text
END
```

- Protocol commands are uppercase.
- The client may accept lowercase user commands, but it must translate them into the corresponding uppercase protocol command before sending them to the server.

Example:

```text
start
```

is sent to the server as:

```text
START
```

---

## 2. Supported Commands

### REGISTER

Registers or authenticates a user.

```text
REGISTER <nickname> <password>
```

Example:

```text
REGISTER pietro ciao123
```

Possible responses:

```text
OK authenticated
ERR ...
```

---

### LOGIN

Authenticates an existing user.

```text
LOGIN <nickname> <password>
```

Example:

```text
LOGIN pietro ciao123
```

Possible responses:

```text
OK authenticated
ERR ...
```

---

### LIST

Requests the list of connected or known users.

```text
LIST
```

Response format:

```text
USERS <n>
<nickname_1>
<nickname_2>
...
END
```

Example:

```text
USERS 2
pietro
mario
END
```

---

### READY

Marks the client as ready while the session is still in the lobby.

```text
READY
```

Response:

```text
OK ready
```

Note: in the current implementation, readiness is tracked, but it does not automatically start the game.

---

### START

Starts the game session.

```text
START
```

Rules:

- only the session owner can execute `START`;
- the first connected client becomes the session owner;
- if the session has already started, the server rejects the command;
- when the session starts, the server generates the maze and moves to the `PLAYING` state.

Possible responses:

```text
SESSION STARTED
ERR only owner can start
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

Examples:

```text
MOVE UP
MOVE DOWN
MOVE LEFT
MOVE RIGHT
```

Possible responses:

```text
MAP LOCAL <rows> <cols>
...
END
```

or:

```text
ERR game not started
ERR game finished
ERR invalid direction
ERR BLOCKED
```

If an object is collected, the server may also send:

```text
OK object collected
```

If the exit is reached, the server may send:

```text
OK exit found
SESSION ENDED
```

---

### LOCAL

Requests the local map around the player.

```text
LOCAL
```

This command is valid only while the session is in the `PLAYING` state.

Response format:

```text
MAP LOCAL <rows> <cols>
<row_1>
<row_2>
...
END
```

Example:

```text
MAP LOCAL 5 5
#####
#   #
# P #
#   #
#####
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

This command is valid only while the session is in the `PLAYING` state.

Response format:

```text
MAP GLOBAL <rows> <cols>
<row_1>
<row_2>
...
END
```

Example:

```text
MAP GLOBAL 21 21
?????????????????????
?P   ????????????????
?????????????????????
...
END
```

Important rule:

- the current player's position must be displayed with `P`;
- `P` has priority over hidden or visible cell rendering.

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

Response format:

```text
RANK <n>
<position> <nickname> <objects> objects [exit]
...
END
```

Example:

```text
RANK 3
1 pietro 5 objects exit
2 mario 3 objects
3 lisa 0 objects
END
```

Sorting rules:

1. players who reached the exit are ranked before players who did not;
2. among players who reached the exit, the earliest exit time wins;
3. if needed, players with more collected objects are ranked higher.

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

Generic successful response.

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

Generic error response.

```text
ERR <message>
```

Examples:

```text
ERR game not started
ERR game already started
ERR game finished
ERR only owner can start
ERR invalid direction
ERR BLOCKED
```

---

### SESSION

Notifies the client about a session state transition.

```text
SESSION STARTED
SESSION ENDED
```

Meaning:

- `SESSION STARTED`: the game session has started;
- `SESSION ENDED`: the game session has ended because of the timer, exit condition, or session termination condition.

---

### USERS

Multiline response containing the user list.

```text
USERS <n>
<nickname_1>
<nickname_2>
...
END
```

---

### MAP LOCAL

Multiline response containing the local map.

```text
MAP LOCAL <rows> <cols>
<row_1>
<row_2>
...
END
```

---

### MAP GLOBAL

Multiline response containing the masked global map.

```text
MAP GLOBAL <rows> <cols>
<row_1>
<row_2>
...
END
```

---

### RANK

Multiline response containing the ranking.

```text
RANK <n>
<position> <nickname> <objects> objects [exit]
...
END
```

---

## 4. Server Session States

The server manages three session states:

```c
typedef enum {
    LOBBY,
    PLAYING,
    FINISHED
} session_state_t;
```

---

### LOBBY

Initial state.

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
```

Typical response:

```text
ERR game not started
```

---

### PLAYING

Active game state.

Allowed commands:

```text
MOVE
LOCAL
GLOBAL
LIST
RANK
QUIT
```

`START` is rejected:

```text
ERR game already started
```

During this state:

- the maze is generated;
- movement is enabled;
- the session timer is active;
- the server may send periodic global map updates.

---

### FINISHED

Final session state.

Allowed commands:

```text
RANK
LIST
QUIT
```

Blocked commands:

```text
MOVE
LOCAL
GLOBAL
START
```

Typical response:

```text
ERR game finished
```

---

## 5. Client States

The client keeps session states consistent with the server:

```c
typedef enum {
    CLIENT_LOBBY,
    CLIENT_PLAYING,
    CLIENT_FINISHED
} client_session_state_t;
```

The client also supports two input modes:

```c
typedef enum {
    MOVEMENT,
    COMMAND
} input_mode_t;
```

---

### MOVEMENT

Fast gameplay mode.

Keys:

```text
w -> MOVE UP
a -> MOVE LEFT
s -> MOVE DOWN
d -> MOVE RIGHT
g/G -> toggle global overlay and request GLOBAL
l/L -> request LOCAL
q -> QUIT
TAB -> switch to COMMAND mode
```

---

### COMMAND

Text command mode.

Examples:

```text
register pietro ciao123
login pietro ciao123
users
ready
start
rank
local
global
quit
```

Slash commands are also supported:

```text
/register
/login
/users
/ready
/start
/rank
/local
/global
/quit
```

`TAB` switches back to `MOVEMENT` mode.

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

Session timer:

```c
#define SESSION_DURATION 300
```

---

## 7. Typical Execution Sequence

Owner client:

```text
REGISTER pietro ciao123
LIST
READY
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
REGISTER mario ciao123
START
READY
LIST
RANK
QUIT
```

Expected response to `START` from the second client:

```text
ERR only owner can start
```

---

## 8. Implementation Notes

- The server owns the game logic.
- The client owns rendering, terminal input, and local display state.
- `termios` is used only on the client side.
- The server does not use `termios`.
- Communication remains textual and human-readable.
- Every multiline response must end with `END`.

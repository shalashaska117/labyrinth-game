# Application Protocol

This project uses a line-oriented text protocol over TCP sockets.

## Client commands

```text
REGISTER nickname password
LOGIN nickname password
MOVE UP
MOVE DOWN
MOVE LEFT
MOVE RIGHT
LOCAL
GLOBAL
LIST
QUIT
```

## Server responses

```text
OK message
ERR message
MAP LOCAL rows cols
<rows>
END
MAP GLOBAL rows cols
<rows>
END
USERS count
<nicknames>
END
```

*This project has been created as part of the 42 curriculum by zoentifi, abenba, yaaitmou.*

# FT_IRC

## Description
FT_IRC is a single-threaded IRC server written in C++98 for the 42 curriculum.
It uses non-blocking sockets and `poll()` to manage multiple clients at the same time, and it implements a practical subset of IRC so users can connect, authenticate, join channels, chat privately, and manage channel state.

The server supports core IRC registration and channel features such as `PASS`, `NICK`, `USER`, `JOIN`, `PART`, `PRIVMSG`, `KICK`, `INVITE`, `TOPIC`, `MODE`, `WHO`, `PING`, and `QUIT`.
It also includes a bonus bot executable that can join the server and respond to simple chat commands.

## Instructions
### Compilation
Build the main server with:

```bash
make
```

Build the bonus bot with:

```bash
make Bonus
```

Clean object files with:

```bash
make clean
```

Remove binaries and objects with:

```bash
make fclean
```

Rebuild everything with:

```bash
make re
```

### Execution
Start the IRC server with:

```bash
./ircserv <port> <password>
```

Start the bonus bot with:

```bash
./ircBOT <host> <port> <password>
```

The server expects exactly two arguments: a valid port number and a non-empty password.
The bot connects to the server, authenticates with the same password, and can react to commands such as `!joke`, `!hp`, `!manual`, `!draw <animal>`, and invite handling.

## Resources
### IRC and networking references
* RFC 1459 - Internet Relay Chat Protocol: https://www.rfc-editor.org/rfc/rfc1459
* IRC command overview: https://modern.ircdocs.horse/
* `poll(2)` manual page: https://man7.org/linux/man-pages/man2/poll.2.html
* `socket(2)` manual page: https://man7.org/linux/man-pages/man2/socket.2.html
* `bind(2)` manual page: https://man7.org/linux/man-pages/man2/bind.2.html
* `listen(2)` manual page: https://man7.org/linux/man-pages/man2/listen.2.html
* `accept(2)` manual page: https://man7.org/linux/man-pages/man2/accept.2.html

### AI usage
AI was used to read and analyze the codebase, identify the supported server and bot features, and draft this README structure and wording.
It was not used to implement protocol logic or to change the server behavior.

### Project-specific notes
* The server executable is named `ircserv`.
* The bonus executable is named `ircBOT`.
* The project targets `-std=c++98`.
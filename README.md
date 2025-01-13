# cnctty

A simple nakenchat client built in C.

## About

I tried to separate the user interface from the main routines logic in my previous client, and got to `cnctty`.

## Features

- Client is written entirely with basic terminal control library `termios` and `ansi` escape codes for colors.
- Custom widgets based UI, having three component: a display that holds messages history, an infobar showing basic status information, and a prompt for user input.
- Automatic selection of username and password at startup.
- New control commands starting with semi-colon.
- Vim-like motion control for scrolling the display or moving inside the prompt:

## Getting Started

### Prerequisites

- C compiler. This project was developed using gcc on Linux.
- openssl development library.
- crypto development library

### Building

```bash
$ git clone --recurse-submodules https://github.com/panv-00/cnctty.git
...

$ cd cnctty
...

$ make
make[1]: Entering directory './cnctty/build'
gcc -o ../cnctty ../src/cnctty.c \
	cnc_library.o cnc_net.o cnc_message.o  \
	-Wall -O3 -g -lssl -lcrypto -pthread \
	
make[1]: Leaving directory './cnctty/build'

# now you can run with:
$ ./cnctty

# or
$ ./cnctty --version
# display version number and exit

# copy this executable anywhere you like (i.e. ~/.bin or ~/.local/bin)
```

## Basic Usage

**key shortcuts**
| Mode       | key                            | Widget  | Action                        |
| ---------- | ------------------------------ | ------- | ----------------------------- |
| **INSERT** | <kbd>ESC</kbd>                 | any     | Switch to **CMD** mode        |
| **INSERT** | <kbd>Ctrl</kbd> + <kbd>c</kbd> | any     | Switch to **CMD** mode        |
| **INSERT** | <kbd>LEFT</kbd>                | PROMPT  | Move cursor left              |
| **INSERT** | <kbd>RIGHT</kbd>               | PROMPT  | Move cursor right             |
| **CMD**    | <kbd>INS</kbd>                 | any     | Switch to **INSERT** mode     |
| **CMD**    | <kbd>i</kbd>                   | any     | Switch to **INSERT** mode     |
| **CMD**    | <kbd>a</kbd>                   | PROMPT  | Append in **INSERT** mode     |
| **CMD**    | <kbd>A</kbd>                   | PROMPT  | Append end in **INSERT** mode |
| **CMD**    | <kbd>0</kbd>                   | PROMPT  | Move cursor to beginning      |
| **CMD**    | <kbd>$</kbd>                   | PROMPT  | Move cursor to end            |
| **CMD**    | <kbd>k</kbd>                   | DISPLAY | Scroll up                     |
| **CMD**    | <kbd>j</kbd>                   | DISPLAY | Scroll down                   |
| **CMD**    | <kbd>l</kbd>                   | PROMPT  | Move cursor right             |
| **CMD**    | <kbd>h</kbd>                   | PROMPT  | Move cursor left              |
| **CMD**    | <kbd>e</kbd>                   | PROMPT  | Move word right               |
| **CMD**    | <kbd>b</kbd>                   | PROMPT  | Move word left                |
| **CMD**    | <kbd>0</kbd>                   | DISPLAY | Scroll to first               |
| **CMD**    | <kbd>$</kbd>                   | DISPLAY | Scroll to last                |
| **CMD**    | <kbd>x</kbd>                   | PROMPT  | Delete under cursor           |
| **CMD**    | <kbd>PAGE UP</kbd>             | DISPLAY | Scroll one page up            |
| **CMD**    | <kbd>PAGE DOWN</kbd>           | DISPLAY | Scroll one page down          |

**PROMPT commands**
| Mode       | Command        | Action                |
| ---------- | -------------- | --------------------- |
| **INSERT** | `:q `          | quit                  |
| **INSERT** | `:Q `          | quit                  |
| **INSERT** | `:c `          | connect to server     |
| **INSERT** | `:C `          | connect to server     |
| **INSERT** | `:d `          | disconnect            |
| **INSERT** | `:D `          | disconnect            |
| **CMD**    | <kbd>q</kbd>   | quit                  |
| **CMD**    | <kbd>Q</kbd>   | quit                  |
| **CMD**    | <kbd>c</kbd>   | connect to server     |
| **CMD**    | <kbd>C</kbd>   | connect to server     |
| **CMD**    | <kbd>d</kbd>   | disconnect            |
| **CMD**    | <kbd>D</kbd>   | disconnect            |

## License

This project is licensed under the MIT License - see the LICENSE file for details.

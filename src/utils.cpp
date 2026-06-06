#include "utils.hpp"
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <cstdio>
#include <cerrno>

void enableRawMode(TerminalState& state) {
    if (tcgetattr(STDIN_FILENO, &state.orig_termios) == -1) {
        die("tcgetattr");
    }
    struct termios raw = state.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
    write(STDOUT_FILENO, "\x1b[?1000h", 8);
}

void disableRawMode(const TerminalState& state) {
    write(STDOUT_FILENO, "\x1b[?1000l", 8);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.orig_termios);
}

void die(const std::string& message) {
    write(STDOUT_FILENO, "\x1b[?1000l", 8);
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    std::perror(message.c_str());
    std::exit(1);
}

bool getWindowSize(int& rows, int& cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return false;
        char buf[32];
        unsigned int i = 0;
        while (i < sizeof(buf) - 1) {
            if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
            if (buf[i] == 'R') break;
            i++;
        }
        buf[i] = '\0';
        if (buf[0] != '\x1b' || buf[1] != '[') return false;
        if (std::sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) return false;
        return true;
    } else {
        rows = ws.ws_row;
        cols = ws.ws_col;
        return true;
    }
}

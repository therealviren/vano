#include "input.hpp"
#include "utils.hpp"
#include <unistd.h>
#include <cstdlib>
#include <cerrno>

int readKey(int& mouse_x, int& mouse_y, int& mouse_b) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    if (c == 0x1f) {
        return CTRL_SLASH;
    }
    if (c == 0x1c) {
        return CTRL_BACKSLASH;
    }
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] == 'M') {
                char mbuf[3];
                if (read(STDIN_FILENO, &mbuf[0], 1) != 1) return '\x1b';
                if (read(STDIN_FILENO, &mbuf[1], 1) != 1) return '\x1b';
                if (read(STDIN_FILENO, &mbuf[2], 1) != 1) return '\x1b';
                mouse_b = mbuf[0] - 32;
                mouse_x = mbuf[1] - 32;
                mouse_y = mbuf[2] - 32;
                return MOUSE_EVENT;
            }
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

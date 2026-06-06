#ifndef UTILS_HPP
#define UTILS_HPP

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <termios.h>
#include <string>

struct TerminalState {
    struct termios orig_termios;
};

void enableRawMode(TerminalState& state);
void disableRawMode(const TerminalState& state);
void die(const std::string& message);
bool getWindowSize(int& rows, int& cols);

#endif

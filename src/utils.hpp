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
#include <cstdint>

struct TerminalState {
    struct termios orig_termios;
};

void enableRawMode(TerminalState& state);
void disableRawMode(const TerminalState& state);
void die(const std::string& message);
bool getWindowSize(int& rows, int& cols);

inline uint32_t decodeUTF8(const std::string& s, size_t& i) {
    if (i >= s.size()) return 0;
    uint8_t c1 = static_cast<uint8_t>(s[i++]);
    if (c1 < 0x80) return c1;
    if ((c1 & 0xE0) == 0xC0 && i < s.size()) {
        uint8_t c2 = static_cast<uint8_t>(s[i++]);
        return static_cast<uint32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F));
    }
    if ((c1 & 0xF0) == 0xE0 && i + 1 < s.size()) {
        uint8_t c2 = static_cast<uint8_t>(s[i++]);
        uint8_t c3 = static_cast<uint8_t>(s[i++]);
        return static_cast<uint32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
    }
    if ((c1 & 0xF0) == 0xF0 && i + 2 < s.size()) {
        uint8_t c2 = static_cast<uint8_t>(s[i++]);
        uint8_t c3 = static_cast<uint8_t>(s[i++]);
        uint8_t c4 = static_cast<uint8_t>(s[i++]);
        return static_cast<uint32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F));
    }
    return c1;
}

inline int getCodepointWidth(uint32_t cp) {
    if (cp == 0) return 0;
    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) return 0;
    if ((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) || (cp >= 0xFE20 && cp <= 0xFE2F)) return 0;
    if ((cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2329 && cp <= 0x232A) || (cp >= 0x2E80 && cp <= 0x303E) || (cp >= 0x3040 && cp <= 0xA4CF) || (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE10 && cp <= 0xFE19) || (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF00 && cp <= 0xFF60) || (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x20000 && cp <= 0x2FA1F) || (cp >= 0x1F300 && cp <= 0x1F9FF) || (cp >= 0x1F600 && cp <= 0x1F64F)) return 2;
    return 1;
}

#endif

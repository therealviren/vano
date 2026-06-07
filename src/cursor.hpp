#ifndef CURSOR_HPP
#define CURSOR_HPP

struct Cursor {
    int cx;
    int cy;
    int rx;
    int rowoff;
    int coloff;

    Cursor();
    void clamp(int num_lines, int line_len);
};

#endif

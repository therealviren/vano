#ifndef INPUT_HPP
#define INPUT_HPP

enum EditorKey {
    TAB_KEY = 9,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
    CTRL_SLASH,
    CTRL_BACKSLASH,
    MOUSE_EVENT
};

int readKey(int& mouse_x, int& mouse_y, int& mouse_b);

#endif

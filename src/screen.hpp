#ifndef SCREEN_HPP
#define SCREEN_HPP

#include "buffer.hpp"
#include "cursor.hpp"
#include <string>

enum EditorMode {
    MODE_NORMAL,
    MODE_VISUAL,
    MODE_SEARCH,
    MODE_COMMAND
};

class Screen {
public:
    int screen_rows;
    int screen_cols;
    std::string output_buffer;
    bool show_gutter;
    int tab_size;

    Screen();
    void init();
    void refresh(const Buffer& buffer, Cursor& cursor, const std::string& filename, const std::string& status_msg, EditorMode mode, int sel_sx, int sel_sy, int sel_ex, int sel_ey);
    void drawRows(const Buffer& buffer, const Cursor& cursor, const std::string& filename, EditorMode mode, int sel_sx, int sel_sy, int sel_ex, int sel_ey);
    void drawStatusBar(const Buffer& buffer, const Cursor& cursor, const std::string& filename, EditorMode mode);
    void drawMessageBar(const std::string& status_msg);
    void scroll(Cursor& cursor, const Buffer& buffer);
    int getGutterWidth(size_t num_lines) const;
    void checkBracketMatch(const Buffer& buffer, const Cursor& cursor, int& match_x, int& match_y);
};

#endif

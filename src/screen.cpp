#include "screen.hpp"
#include "utils.hpp"
#include "version.hpp"
#include <unistd.h>
#include <algorithm>
#include <cstdio>

Screen::Screen() : screen_rows(0), screen_cols(0), show_gutter(true), tab_size(4) {}

void Screen::init() {
    if (!getWindowSize(screen_rows, screen_cols)) {
        die("getWindowSize");
    }
    screen_rows -= 2;
}

int Screen::getGutterWidth(size_t num_lines) const {
    if (!show_gutter) return 0;
    return std::to_string(num_lines).size() + 2;
}

void Screen::scroll(Cursor& cursor, const Buffer& buffer) {
    cursor.rx = 0;
    int gutter = getGutterWidth(buffer.lines.size());
    int usable_cols = screen_cols - gutter;

    if (cursor.cy < static_cast<int>(cursor.rowoff)) {
        cursor.rowoff = cursor.cy;
    }
    if (cursor.cy >= static_cast<int>(cursor.rowoff) + screen_rows) {
        cursor.rowoff = cursor.cy - screen_rows + 1;
    }
    if (cursor.cx < static_cast<int>(cursor.coloff)) {
        cursor.coloff = cursor.cx;
    }
    if (cursor.cx >= static_cast<int>(cursor.coloff) + usable_cols) {
        cursor.coloff = cursor.cx - usable_cols + 1;
    }
}

void Screen::checkBracketMatch(const Buffer& buffer, const Cursor& cursor, int& match_x, int& match_y) {
    match_x = -1;
    match_y = -1;
    if (cursor.cy < 0 || cursor.cy >= static_cast<int>(buffer.lines.size())) return;
    if (cursor.cx < 0 || cursor.cx >= static_cast<int>(buffer.lines[cursor.cy].size())) return;

    char current = buffer.lines[cursor.cy][cursor.cx];
    int dir = 0;
    char target = 0;

    if (current == '{') { dir = 1; target = '}'; }
    else if (current == '}') { dir = -1; target = '{'; }
    else if (current == '[') { dir = 1; target = ']'; }
    else if (current == ']') { dir = -1; target = '['; }
    else if (current == '(') { dir = 1; target = ')'; }
    else if (current == ')') { dir = -1; target = '('; }

    if (dir == 0) return;

    int depth = 1;
    int y = cursor.cy;
    int x = cursor.cx + dir;

    while (y >= 0 && y < static_cast<int>(buffer.lines.size())) {
        if (x < 0 || x >= static_cast<int>(buffer.lines[y].size())) {
            y += dir;
            if (y >= 0 && y < static_cast<int>(buffer.lines.size())) {
                x = (dir == 1) ? 0 : buffer.lines[y].size() - 1;
            }
            continue;
        }
        if (buffer.lines[y][x] == current) depth++;
        else if (buffer.lines[y][x] == target) depth--;

        if (depth == 0) {
            match_x = x;
            match_y = y;
            return;
        }
        x += dir;
    }
}

void Screen::drawRows(const Buffer& buffer, const Cursor& cursor, EditorMode mode, int sel_sx, int sel_sy, int sel_ex, int sel_ey) {
    int gutter = getGutterWidth(buffer.lines.size());
    int usable_cols = screen_cols - gutter;
    int bx, by;
    checkBracketMatch(buffer, cursor, bx, by);

    if (mode == MODE_VISUAL) {
        if (sel_sy > sel_ey || (sel_sy == sel_ey && sel_sx > sel_ex)) {
            std::swap(sel_sx, sel_ex);
            std::swap(sel_sy, sel_ey);
        }
    }

    for (int y = 0; y < screen_rows; y++) {
        int filerow = y + cursor.rowoff;
        if (show_gutter) {
            output_buffer += "\x1b[90m";
            if (filerow < static_cast<int>(buffer.lines.size())) {
                char gbuf[16];
                std::snprintf(gbuf, sizeof(gbuf), "%*d ", gutter - 1, filerow + 1);
                output_buffer += gbuf;
            } else {
                for (int i = 0; i < gutter - 1; i++) output_buffer += " ";
                output_buffer += " ";
            }
            output_buffer += "\x1b[m";
        }

        if (filerow >= static_cast<int>(buffer.lines.size())) {
            if (buffer.lines.size() == 1 && buffer.lines[0].empty() && y == screen_rows / 3) {
                std::string welcome = "Vano editor - version " VANO_VERSION;
                int welcomelen = welcome.size();
                if (welcomelen > usable_cols) welcomelen = usable_cols;
                int padding = (usable_cols - welcomelen) / 2;
                if (padding) {
                    output_buffer += "~";
                    padding--;
                }
                while (padding--) output_buffer += " ";
                output_buffer += welcome.substr(0, welcomelen);
            } else {
                output_buffer += "~";
            }
        } else {
            int len = static_cast<int>(buffer.lines[filerow].size()) - cursor.coloff;
            if (len < 0) len = 0;
            if (len > usable_cols) len = usable_cols;

            if (len == 0 && mode == MODE_VISUAL) {
                bool is_selected = false;
                if (filerow > sel_sy && filerow < sel_ey) is_selected = true;
                else if (filerow == sel_sy && filerow == sel_ey) is_selected = (cursor.coloff >= sel_sx && cursor.coloff <= sel_ex);
                else if (filerow == sel_sy) is_selected = (cursor.coloff >= sel_sx);
                else if (filerow == sel_ey) is_selected = (cursor.coloff <= sel_ex);

                if (is_selected) {
                    output_buffer += "\x1b[7m \x1b[m";
                }
            }

            for (int i = 0; i < len; i++) {
                int real_idx = i + cursor.coloff;
                bool is_selected = false;

                if (mode == MODE_VISUAL) {
                    if (filerow > sel_sy && filerow < sel_ey) is_selected = true;
                    else if (filerow == sel_sy && filerow == sel_ey) is_selected = (real_idx >= sel_sx && real_idx <= sel_ex);
                    else if (filerow == sel_sy) is_selected = (real_idx >= sel_sx);
                    else if (filerow == sel_ey) is_selected = (real_idx <= sel_ex);
                }

                bool is_bracket = (filerow == by && real_idx == bx) || (filerow == cursor.cy && real_idx == cursor.cx && bx != -1);

                if (is_selected) output_buffer += "\x1b[7m";
                if (is_bracket) output_buffer += "\x1b[1;44m";

                output_buffer += buffer.lines[filerow][real_idx];

                if (is_bracket) output_buffer += "\x1b[m";
                if (is_selected) output_buffer += "\x1b[m";
            }
        }
        output_buffer += "\x1b[K\r\n";
    }
}

void Screen::drawStatusBar(const Buffer& buffer, const Cursor& cursor, const std::string& filename, EditorMode mode) {
    if (mode == MODE_NORMAL) output_buffer += "\x1b[44;37m";
    else if (mode == MODE_VISUAL) output_buffer += "\x1b[45;37m";
    else if (mode == MODE_SEARCH) output_buffer += "\x1b[42;37m";

    std::string mstr = " [NORMAL] ";
    if (mode == MODE_VISUAL) mstr = " [VISUAL] ";
    if (mode == MODE_SEARCH) mstr = " [SEARCH] ";

    std::string fname = filename.empty() ? "[No Name]" : filename;
    std::string status = mstr + fname + " - " + std::to_string(buffer.lines.size()) + " lines" + (buffer.dirty ? " *" : "");
    std::string rstatus = std::to_string(cursor.cy + 1) + ":" + std::to_string(cursor.cx + 1) + " ";
    int len = status.size();
    int rlen = rstatus.size();
    if (len > screen_cols) len = screen_cols;
    output_buffer += status.substr(0, len);
    while (len < screen_cols) {
        if (screen_cols - len == rlen) {
            output_buffer += rstatus;
            break;
        } else {
            output_buffer += " ";
            len++;
        }
    }
    output_buffer += "\x1b[m\r\n";
}

void Screen::drawMessageBar(const std::string& status_msg) {
    output_buffer += "\x1b[K";
    int len = status_msg.size();
    if (len > screen_cols) len = screen_cols;
    if (len > 0) output_buffer += status_msg.substr(0, len);
}

void Screen::refresh(const Buffer& buffer, Cursor& cursor, const std::string& filename, const std::string& status_msg, EditorMode mode, int sel_sx, int sel_sy, int sel_ex, int sel_ey) {
    scroll(cursor, buffer);
    output_buffer.clear();
    output_buffer += "\x1b[?25l";
    output_buffer += "\x1b[H";
    drawRows(buffer, cursor, mode, sel_sx, sel_sy, sel_ex, sel_ey);
    drawStatusBar(buffer, cursor, filename, mode);
    drawMessageBar(status_msg);
    char buf[32];
    int gutter = getGutterWidth(buffer.lines.size());
    std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (cursor.cy - cursor.rowoff) + 1, (cursor.cx - cursor.coloff) + 1 + gutter);
    output_buffer += buf;
    output_buffer += "\x1b[?25h";
    write(STDOUT_FILENO, output_buffer.c_str(), output_buffer.size());
}

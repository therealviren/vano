#include "screen.hpp"
#include "utils.hpp"
#include "version.hpp"
#include <unistd.h>
#include <algorithm>
#include <string>
#include <vector>
#include <cctype>

Screen::Screen() : screen_rows(0), screen_cols(0), show_gutter(true), tab_size(4) {}

void Screen::init() {
    int temp_rows = 0, temp_cols = 0;
    if (!getWindowSize(temp_rows, temp_cols)) {
        die("getWindowSize");
    }
    if (temp_rows < 5) temp_rows = 5;
    if (temp_cols < 15) temp_cols = 15;
    screen_rows = temp_rows - 2;
    screen_cols = temp_cols;
}

int Screen::getGutterWidth(size_t num_lines) const {
    if (!show_gutter) return 0;
    size_t safe_lines = num_lines > 1 ? num_lines : 1;
    return std::to_string(safe_lines).size() + 2;
}

void Screen::scroll(Cursor& cursor, const Buffer& buffer) {
    int visual_cx = 0;
    int eff_tab = tab_size > 0 ? tab_size : 1;

    if (cursor.cy < 0) cursor.cy = 0;
    if (cursor.cy >= static_cast<int>(buffer.lines.size())) {
        cursor.cy = buffer.lines.empty() ? 0 : buffer.lines.size() - 1;
    }

    if (cursor.cy >= 0 && cursor.cy < static_cast<int>(buffer.lines.size())) {
        const std::string& line = buffer.lines[cursor.cy];
        if (cursor.cx < 0) cursor.cx = 0;
        if (cursor.cx > static_cast<int>(line.size())) cursor.cx = line.size();

        size_t i = 0;
        while (i < line.size() && static_cast<int>(i) < cursor.cx) {
            if (line[i] == '\t') {
                visual_cx += (eff_tab - (visual_cx % eff_tab));
                i++;
            } else {
                uint32_t cp = decodeUTF8(line, i);
                visual_cx += getCodepointWidth(cp);
            }
        }
    }
    cursor.rx = visual_cx;

    int gutter = getGutterWidth(buffer.lines.size());
    int usable_cols = screen_cols - gutter;
    if (usable_cols < 1) usable_cols = 1;
    if (screen_rows < 1) screen_rows = 1;

    if (cursor.cy < static_cast<int>(cursor.rowoff)) {
        cursor.rowoff = cursor.cy;
    }
    if (cursor.cy >= static_cast<int>(cursor.rowoff) + screen_rows) {
        cursor.rowoff = cursor.cy - screen_rows + 1;
    }
    if (visual_cx < static_cast<int>(cursor.coloff)) {
        cursor.coloff = visual_cx;
    }
    if (visual_cx >= static_cast<int>(cursor.coloff) + usable_cols) {
        cursor.coloff = visual_cx - usable_cols + 1;
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
    bool in_string = false;
    bool in_char = false;

    while (y >= 0 && y < static_cast<int>(buffer.lines.size())) {
        if (x < 0 || x >= static_cast<int>(buffer.lines[y].size())) {
            y += dir;
            if (y >= 0 && y < static_cast<int>(buffer.lines.size())) {
                x = (dir == 1) ? 0 : buffer.lines[y].size() - 1;
            }
            in_string = false;
            in_char = false;
            continue;
        }

        char c = buffer.lines[y][x];

        if (c == '"' && (x == 0 || buffer.lines[y][x - 1] != '\\') && !in_char) in_string = !in_string;
        if (c == '\'' && (x == 0 || buffer.lines[y][x - 1] != '\\') && !in_string) in_char = !in_char;

        if (!in_string && !in_char) {
            if (c == current) depth++;
            else if (c == target) depth--;

            if (depth == 0) {
                match_x = x;
                match_y = y;
                return;
            }
        }
        x += dir;
    }
}

void Screen::drawRows(const Buffer& buffer, const Cursor& cursor, const std::string& filename, EditorMode mode, int sel_sx, int sel_sy, int sel_ex, int sel_ey) {
    int gutter = getGutterWidth(buffer.lines.size());
    int usable_cols = screen_cols - gutter;
    if (usable_cols < 1) usable_cols = 1;
    
    int bx, by;
    checkBracketMatch(buffer, cursor, bx, by);

    size_t ext_idx = filename.rfind('.');
    std::string ext = (ext_idx == std::string::npos) ? "" : filename.substr(ext_idx);

    std::vector<std::string> keywords;
    std::vector<std::string> types;
    
    if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".cc" || ext == ".c") {
        keywords = {"if", "else", "return", "while", "for", "class", "struct", "public", "private", "protected", "static", "const", "namespace", "using", "new", "delete", "break", "continue", "switch", "case", "default"};
        types = {"int", "void", "char", "double", "float", "bool", "size_t", "uint32_t", "auto", "std"};
    } else if (ext == ".py") {
        keywords = {"def", "class", "if", "else", "elif", "return", "import", "from", "while", "for", "in", "try", "except", "lambda", "with", "as", "pass", "True", "False", "None", "break", "continue", "and", "or", "not", "is"};
        types = {"int", "str", "float", "bool", "list", "dict", "set", "tuple"};
    } else if (ext == ".sh" || ext == ".bash") {
        keywords = {"if", "then", "else", "fi", "for", "while", "do", "done", "echo", "exit", "return", "local", "case", "esac", "export", "function", "read"};
        types = {};
    } else if (ext == ".js" || ext == ".ts") {
        keywords = {"if", "else", "return", "while", "for", "class", "function", "const", "let", "var", "import", "export", "from", "switch", "case", "break", "continue", "new", "true", "false", "null", "undefined", "await", "async"};
        types = {"Number", "String", "Boolean", "Object", "Array", "Promise", "any"};
    }

    if (mode == MODE_VISUAL) {
        if (sel_sy > sel_ey || (sel_sy == sel_ey && sel_sx > sel_ex)) {
            std::swap(sel_sx, sel_ex);
            std::swap(sel_sy, sel_ey);
        }
    }

    int eff_tab = tab_size > 0 ? tab_size : 1;

    for (int y = 0; y < screen_rows; y++) {
        int filerow = y + cursor.rowoff;
        
        if (show_gutter) {
            output_buffer += "\x1b[90m";
            if (filerow < static_cast<int>(buffer.lines.size())) {
                std::string gnum = std::to_string(filerow + 1);
                int padding = (gutter - 1) - gnum.size();
                if (padding < 0) padding = 0;
                for (int p = 0; p < padding; p++) output_buffer += " ";
                output_buffer += gnum + " ";
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
                if (padding > 0) {
                    output_buffer += "~";
                    padding--;
                }
                while (padding > 0) {
                    output_buffer += " ";
                    padding--;
                }
                output_buffer += welcome.substr(0, welcomelen);
            } else {
                output_buffer += "~";
            }
        } else {
            int line_size = static_cast<int>(buffer.lines[filerow].size());
            std::vector<std::string> syntax_colors(line_size, "");
            std::string current_word = "";
            std::vector<int> current_word_indices;
            bool in_string = false;
            char string_char = 0;

            for (int i = 0; i < line_size; ++i) {
                char c = buffer.lines[filerow][i];

                if (!in_string && (c == '"' || c == '\'' || c == '`')) {
                    in_string = true;
                    string_char = c;
                    syntax_colors[i] = "\x1b[33m";
                    continue;
                }
                if (in_string && c == string_char && (i == 0 || buffer.lines[filerow][i - 1] != '\\')) {
                    in_string = false;
                    syntax_colors[i] = "\x1b[33m";
                    continue;
                }
                if (in_string) {
                    syntax_colors[i] = "\x1b[33m";
                    continue;
                }

                if (c == '#') {
                    for (int j = i; j < line_size; j++) {
                        syntax_colors[j] = "\x1b[35m";
                    }
                    break;
                }

                if (std::string("+-*/=<>!&|%^~?").find(c) != std::string::npos) {
                    syntax_colors[i] = "\x1b[38;5;208m";
                    continue;
                }

                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                    current_word += c;
                    current_word_indices.push_back(i);
                } else {
                    if (!current_word.empty()) {
                        bool is_keyword = std::find(keywords.begin(), keywords.end(), current_word) != keywords.end();
                        bool is_type = std::find(types.begin(), types.end(), current_word) != types.end();
                        bool is_number = std::isdigit(static_cast<unsigned char>(current_word[0]));
                        
                        std::string color = "";
                        if (is_keyword) color = "\x1b[32m";
                        else if (is_type) color = "\x1b[34m";
                        else if (is_number) color = "\x1b[36m";

                        if (!color.empty()) {
                            for (int idx : current_word_indices) {
                                syntax_colors[idx] = color;
                            }
                        }
                        current_word.clear();
                        current_word_indices.clear();
                    }
                }
            }
            if (!current_word.empty()) {
                bool is_keyword = std::find(keywords.begin(), keywords.end(), current_word) != keywords.end();
                bool is_type = std::find(types.begin(), types.end(), current_word) != types.end();
                bool is_number = std::isdigit(static_cast<unsigned char>(current_word[0]));
                
                std::string color = "";
                if (is_keyword) color = "\x1b[32m";
                else if (is_type) color = "\x1b[34m";
                else if (is_number) color = "\x1b[36m";

                if (!color.empty()) {
                    for (int idx : current_word_indices) {
                        syntax_colors[idx] = color;
                    }
                }
            }

            struct VisualCell {
                std::string bytes;
                int byte_idx;
                bool is_tab_pad;
                int width;
            };
            
            std::vector<VisualCell> cells;
            int current_vis_x = 0;
            
            for (size_t i = 0; i < buffer.lines[filerow].size();) {
                if (buffer.lines[filerow][i] == '\t') {
                    int spaces = eff_tab - (current_vis_x % eff_tab);
                    cells.push_back({"\t", static_cast<int>(i), false, spaces});
                    for (int t = 1; t < spaces; ++t) {
                        cells.push_back({" ", static_cast<int>(i), true, 0});
                    }
                    current_vis_x += spaces;
                    i++;
                } else {
                    size_t start_idx = i;
                    uint32_t cp = decodeUTF8(buffer.lines[filerow], i);
                    std::string utf8_char = buffer.lines[filerow].substr(start_idx, i - start_idx);
                    int w = getCodepointWidth(cp);
                    
                    if (w == 0) {
                        if (!cells.empty()) {
                            cells.back().bytes += utf8_char;
                        } else {
                            cells.push_back({utf8_char, static_cast<int>(start_idx), false, 0});
                        }
                    } else {
                        cells.push_back({utf8_char, static_cast<int>(start_idx), false, w});
                        for (int w_idx = 1; w_idx < w; ++w_idx) {
                            cells.push_back({"", static_cast<int>(start_idx), true, 0});
                        }
                        current_vis_x += w;
                    }
                }
            }

            if (cells.empty() && mode == MODE_VISUAL) {
                bool is_selected = false;
                if (filerow > sel_sy && filerow < sel_ey) is_selected = true;
                else if (filerow == sel_sy && filerow == sel_ey) is_selected = (cursor.coloff >= sel_sx && cursor.coloff <= sel_ex);
                else if (filerow == sel_sy) is_selected = (cursor.coloff >= sel_sx);
                else if (filerow == sel_ey) is_selected = (cursor.coloff <= sel_ex);

                if (is_selected) {
                    output_buffer += "\x1b[7m \x1b[m";
                }
            }

            for (int c = cursor.coloff; c < cursor.coloff + usable_cols; ++c) {
                if (c < static_cast<int>(cells.size())) {
                    const auto& cell = cells[c];
                    int real_idx = cell.byte_idx;
                    bool is_selected = false;

                    if (mode == MODE_VISUAL) {
                        if (filerow > sel_sy && filerow < sel_ey) is_selected = true;
                        else if (filerow == sel_sy && filerow == sel_ey) is_selected = (real_idx >= sel_sx && real_idx <= sel_ex);
                        else if (filerow == sel_sy) is_selected = (real_idx >= sel_sx);
                        else if (filerow == sel_ey) is_selected = (real_idx <= sel_ex);
                    }

                    bool is_bracket = (filerow == by && real_idx == bx) || (filerow == cursor.cy && real_idx == cursor.cx && bx != -1);
                    std::string syntax_color = cell.is_tab_pad ? "" : syntax_colors[real_idx];

                    if (is_selected) output_buffer += "\x1b[7m";
                    if (is_bracket) output_buffer += "\x1b[1;44m";
                    if (!is_bracket && !syntax_color.empty()) output_buffer += syntax_color;

                    output_buffer += cell.bytes;

                    if (!is_bracket && !syntax_color.empty()) output_buffer += "\x1b[m";
                    if (is_bracket) output_buffer += "\x1b[m";
                    if (is_selected) output_buffer += "\x1b[m";
                } else {
                    break;
                }
            }
        }
        output_buffer += "\x1b[K\r\n";
    }
}

void Screen::drawStatusBar(const Buffer& buffer, const Cursor& cursor, const std::string& filename, EditorMode mode) {
    if (mode == MODE_NORMAL) output_buffer += "\x1b[44;37m";
    else if (mode == MODE_VISUAL) output_buffer += "\x1b[45;37m";
    else if (mode == MODE_SEARCH) output_buffer += "\x1b[42;37m";
    else if (mode == MODE_COMMAND) output_buffer += "\x1b[46;30m";

    std::string mstr = " [NORMAL] ";
    if (mode == MODE_VISUAL) mstr = " [VISUAL] ";
    else if (mode == MODE_SEARCH) mstr = " [SEARCH] ";
    else if (mode == MODE_COMMAND) mstr = " [COMMAND] ";

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
    int temp_rows = 0, temp_cols = 0;
    if (getWindowSize(temp_rows, temp_cols)) {
        if (temp_rows < 5) temp_rows = 5;
        if (temp_cols < 15) temp_cols = 15;
        screen_rows = temp_rows - 2;
        screen_cols = temp_cols;
    }

    scroll(cursor, buffer);
    
    output_buffer.clear();
    output_buffer += "\x1b[?25l";
    output_buffer += "\x1b[H";
    
    drawRows(buffer, cursor, filename, mode, sel_sx, sel_sy, sel_ex, sel_ey);
    drawStatusBar(buffer, cursor, filename, mode);
    drawMessageBar(status_msg);
    
    int gutter = getGutterWidth(buffer.lines.size());
    int visual_cx = 0;
    int eff_tab = tab_size > 0 ? tab_size : 1;

    if (cursor.cy >= 0 && cursor.cy < static_cast<int>(buffer.lines.size())) {
        const std::string& line = buffer.lines[cursor.cy];
        size_t i = 0;
        while (i < line.size() && static_cast<int>(i) < cursor.cx) {
            if (line[i] == '\t') {
                visual_cx += (eff_tab - (visual_cx % eff_tab));
                i++;
            } else {
                uint32_t cp = decodeUTF8(line, i);
                visual_cx += getCodepointWidth(cp);
            }
        }
    }

    int cursor_y = (cursor.cy - cursor.rowoff) + 1;
    int cursor_x = (visual_cx - cursor.coloff) + 1 + gutter;
    
    if (cursor_y < 1) cursor_y = 1;
    if (cursor_x < 1) cursor_x = 1;

    output_buffer += "\x1b[" + std::to_string(cursor_y) + ";" + std::to_string(cursor_x) + "H";
    output_buffer += "\x1b[?25h";
    
    write(STDOUT_FILENO, output_buffer.c_str(), output_buffer.size());
}

#include "editor.hpp"
#include "input.hpp"
#include "file.hpp"
#include "version.hpp"
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <regex>

#define CTRL_KEY(k) ((k) & 0x1f)

Editor::Editor() : quit(false), auto_save_counter(0), mode(MODE_NORMAL), sel_start_x(0), sel_start_y(0) {}

std::string Editor::getFileExtension() const {
    size_t idx = filename.rfind('.');
    if (idx == std::string::npos) return "";
    return filename.substr(idx);
}

void Editor::init(const std::string& file_path) {
    enableRawMode(state);
    screen.init();

    bool ai = true;
    FileManager::loadConfig(screen.tab_size, ai, screen.show_gutter);
    buffer.auto_indent = ai;

    filename = file_path;
    if (!filename.empty()) {
        if (!FileManager::openFile(buffer, filename)) {
            setStatus("New file: " + filename);
        } else {
            setStatus("Opened file: " + filename);
        }
    } else {
        setStatus("Welcome to Vano editor - v" VANO_VERSION);
    }
}

void Editor::setStatus(const std::string& msg) {
    status_msg = msg;
}

void Editor::handleMouse(int mx, int my, int mb) {
    if (mb == 0 || mb == 32) {
        int gutter = screen.getGutterWidth(buffer.lines.size());
        int target_row = my + cursor.rowoff - 1;
        int target_col = mx + cursor.coloff - gutter - 1;

        if (target_row >= 0 && target_row < static_cast<int>(buffer.lines.size())) {
            cursor.cy = target_row;
            if (target_col >= 0 && target_col <= static_cast<int>(buffer.lines[cursor.cy].size())) {
                cursor.cx = target_col;
            } else if (target_col < 0) {
                cursor.cx = 0;
            } else {
                cursor.cx = buffer.lines[cursor.cy].size();
            }
        }
    }
}

void Editor::moveCursor(int key) {
    int line_len = buffer.getLineLength(cursor.cy);
    switch (key) {
        case ARROW_LEFT:
            if (cursor.cx > 0) {
                cursor.cx--;
            } else if (cursor.cy > 0) {
                cursor.cy--;
                cursor.cx = buffer.getLineLength(cursor.cy);
            }
            break;
        case ARROW_RIGHT:
            if (cursor.cx < line_len) {
                cursor.cx++;
            } else if (cursor.cy < static_cast<int>(buffer.lines.size()) - 1) {
                cursor.cy++;
                cursor.cx = 0;
            }
            break;
        case ARROW_UP:
            if (cursor.cy > 0) cursor.cy--;
            break;
        case ARROW_DOWN:
            if (cursor.cy < static_cast<int>(buffer.lines.size()) - 1) cursor.cy++;
            break;
    }
    line_len = buffer.getLineLength(cursor.cy);
    if (cursor.cx > line_len) cursor.cx = line_len;
}

void Editor::findAndReplace() {
    EditorMode old_mode = mode;
    mode = MODE_SEARCH;
    std::string pattern, replace;

    while (true) {
        setStatus("Regex Search: " + pattern + " (Enter to proceed, Esc to cancel)");
        screen.refresh(buffer, cursor, filename, status_msg, mode, 0, 0, 0, 0);
        int mx, my, mb;
        int c = readKey(mx, my, mb);
        if (c == '\x1b') { setStatus(""); mode = old_mode; return; }
        else if (c == '\r' || c == '\n') break;
        else if (c == BACKSPACE || c == DEL_KEY) { if (!pattern.empty()) pattern.pop_back(); }
        else if (!std::iscntrl(c) && c < 128) pattern += c;
    }

    while (true) {
        setStatus("Replace with: " + replace + " (Enter to search, Esc to cancel)");
        screen.refresh(buffer, cursor, filename, status_msg, mode, 0, 0, 0, 0);
        int mx, my, mb;
        int c = readKey(mx, my, mb);
        if (c == '\x1b') { setStatus(""); mode = old_mode; return; }
        else if (c == '\r' || c == '\n') break;
        else if (c == BACKSPACE || c == DEL_KEY) { if (!replace.empty()) replace.pop_back(); }
        else if (!std::iscntrl(c) && c < 128) replace += c;
    }

    try {
        std::regex target_regex(pattern);
        bool replace_all = false;

        for (size_t i = 0; i < buffer.lines.size(); ++i) {
            std::smatch match;
            std::string current_line = buffer.lines[i];
            size_t search_offset = 0;

            while (std::regex_search(current_line.cbegin() + search_offset, current_line.cend(), match, target_regex)) {
                size_t match_pos = search_offset + match.position(0);
                cursor.cy = i;
                cursor.cx = match_pos;

                if (!replace_all) {
                    setStatus("Replace match? (y: Yes, n: No, a: All, q: Quit)");
                    screen.refresh(buffer, cursor, filename, status_msg, mode, match_pos, i, match_pos + match.length(0) - 1, i);
                    int mx, my, mb;
                    int choice = readKey(mx, my, mb);
                    if (choice == 'q' || choice == '\x1b') { setStatus("Cancelled."); mode = old_mode; return; }
                    if (choice == 'a') replace_all = true;
                    if (choice == 'y' || replace_all) {
                        std::string updated = current_line.substr(0, match_pos) + replace + current_line.substr(match_pos + match.length(0));
                        std::vector<std::string> old_blk = { buffer.lines[i] };
                        buffer.lines[i] = updated;
                        std::vector<std::string> new_blk = { updated };
                        buffer.pushBlockUndo(i, i, old_blk, new_blk, match_pos, i, match_pos + replace.size(), i, UndoType::BLOCK_REPLACE);
                        
                        current_line = updated;
                        search_offset = match_pos + replace.size();
                        if (match.length(0) == 0) {
                            search_offset++;
                        }
                        buffer.dirty = true;
                        continue;
                    }
                } else {
                    std::string updated = current_line.substr(0, match_pos) + replace + current_line.substr(match_pos + match.length(0));
                    std::vector<std::string> old_blk = { buffer.lines[i] };
                    buffer.lines[i] = updated;
                    std::vector<std::string> new_blk = { updated };
                    buffer.pushBlockUndo(i, i, old_blk, new_blk, match_pos, i, match_pos + replace.size(), i, UndoType::BLOCK_REPLACE);
                    
                    current_line = updated;
                    search_offset = match_pos + replace.size();
                    if (match.length(0) == 0) {
                        search_offset++;
                    }
                    buffer.dirty = true;
                    continue;
                }
                search_offset = match_pos + (match.length(0) > 0 ? match.length(0) : 1);
            }
        }
    } catch (const std::regex_error&) {
        setStatus("Invalid Regex Pattern.");
    }
    mode = old_mode;
}

void Editor::processKeypress() {
    int mx, my, mb;
    int c = readKey(mx, my, mb);

    if (c == MOUSE_EVENT) {
        handleMouse(mx, my, mb);
        return;
    }

    switch (c) {
        case '\r':
        case '\n':
            buffer.insertNewline(cursor.cy, cursor.cx);
            cursor.cy++;
            cursor.cx = 0;
            break;
        case CTRL_KEY('q'):
            if (buffer.dirty) {
                setStatus("Warning: Unsaved changes. Press Ctrl-Q again to force quit.");
                screen.refresh(buffer, cursor, filename, status_msg, mode, sel_start_x, sel_start_y, cursor.cx, cursor.cy);
                int cmx, cmy, cmb;
                int confirm = readKey(cmx, cmy, cmb);
                if (confirm != CTRL_KEY('q')) { setStatus(""); break; }
            }
            quit = true;
            break;
        case CTRL_KEY('s'):
            if (filename.empty()) {
                while (true) {
                    setStatus("Save as: " + filename + " (Press Enter)");
                    screen.refresh(buffer, cursor, filename, status_msg, mode, 0, 0, 0, 0);
                    int smx, smy, smb;
                    int sc = readKey(smx, smy, smb);
                    if (sc == '\r' || sc == '\n') break;
                    if (!std::iscntrl(sc) && sc < 128) filename += sc;
                }
            }
            if (!filename.empty()) {
                if (FileManager::saveFile(buffer, filename)) setStatus("File saved successfully.");
                else setStatus("Error saving file.");
            }
            break;
        case CTRL_KEY('f'):
            findAndReplace();
            break;
        case CTRL_BACKSLASH:
            findAndReplace();
            break;
        case CTRL_SLASH:
            if (mode == MODE_VISUAL) {
                buffer.toggleComment(sel_start_y, cursor.cy, getFileExtension());
                mode = MODE_NORMAL;
            } else {
                buffer.toggleComment(cursor.cy, cursor.cy, getFileExtension());
            }
            break;
        case CTRL_KEY('z'):
            buffer.undo(cursor.cx, cursor.cy);
            break;
        case CTRL_KEY('r'):
            buffer.redo(cursor.cx, cursor.cy);
            break;
        case CTRL_KEY('v'):
            if (mode == MODE_NORMAL) {
                mode = MODE_VISUAL;
                sel_start_x = cursor.cx;
                sel_start_y = cursor.cy;
            } else {
                mode = MODE_NORMAL;
            }
            break;
        case CTRL_KEY('c'):
            if (mode == MODE_VISUAL) {
                clipboard.clear();
                int sy = sel_start_y, ey = cursor.cy, sx = sel_start_x, ex = cursor.cx;
                if (sy > ey || (sy == ey && sx > ex)) { std::swap(sy, ey); std::swap(sx, ex); }
                for (int i = sy; i <= ey; ++i) {
                    if (i == sy && i == ey) clipboard.push_back(buffer.lines[i].substr(sx, ex - sx + 1));
                    else if (i == sy) clipboard.push_back(buffer.lines[i].substr(sx));
                    else if (i == ey) clipboard.push_back(buffer.lines[i].substr(0, ex + 1));
                    else clipboard.push_back(buffer.lines[i]);
                }
                mode = MODE_NORMAL;
                setStatus("Selected block copied to clipboard.");
            }
            break;
        case CTRL_KEY('x'):
            if (mode == MODE_VISUAL) {
                clipboard.clear();
                int sy = sel_start_y, ey = cursor.cy, sx = sel_start_x, ex = cursor.cx;
                if (sy > ey || (sy == ey && sx > ex)) { std::swap(sy, ey); std::swap(sx, ex); }
                
                std::vector<std::string> old_block;
                for (int i = sy; i <= ey; ++i) {
                    if (i >= 0 && i < static_cast<int>(buffer.lines.size())) {
                        old_block.push_back(buffer.lines[i]);
                    }
                }
                
                for (int i = sy; i <= ey; ++i) {
                    if (i == sy && i == ey) {
                        clipboard.push_back(buffer.lines[i].substr(sx, ex - sx + 1));
                        buffer.lines[i].erase(sx, ex - sx + 1);
                    } else if (i == sy) {
                        clipboard.push_back(buffer.lines[i].substr(sx));
                        buffer.lines[i] = buffer.lines[i].substr(0, sx);
                    } else if (i == ey) {
                        clipboard.push_back(buffer.lines[i].substr(0, ex + 1));
                        buffer.lines[i].erase(0, ex + 1);
                    } else {
                        clipboard.push_back(buffer.lines[i]);
                        buffer.lines[i] = "";
                    }
                }
                if (sy != ey) {
                    buffer.lines[sy] += buffer.lines[ey];
                    buffer.lines.erase(buffer.lines.begin() + sy + 1, buffer.lines.begin() + ey + 1);
                }
                
                std::vector<std::string> new_block = { buffer.lines[sy] };
                buffer.pushBlockUndo(sy, ey, old_block, new_block, cursor.cx, cursor.cy, sx, sy, UndoType::BLOCK_REPLACE);
                
                cursor.cx = sx;
                cursor.cy = sy;
                mode = MODE_NORMAL;
                buffer.dirty = true;
                setStatus("Selected block cut.");
            }
            break;
        case CTRL_KEY('p'):
            if (!clipboard.empty()) {
                std::vector<std::string> old_block = { buffer.lines[cursor.cy] };
                int old_cx = cursor.cx;
                int old_cy = cursor.cy;
                int new_cx = cursor.cx;
                int new_cy = cursor.cy;

                if (clipboard.size() == 1) {
                    buffer.lines[cursor.cy].insert(cursor.cx, clipboard[0]);
                    new_cx += clipboard[0].size();
                    std::vector<std::string> new_block = { buffer.lines[cursor.cy] };
                    buffer.pushBlockUndo(old_cy, old_cy, old_block, new_block, old_cx, old_cy, new_cx, new_cy, UndoType::BLOCK_REPLACE);
                } else {
                    std::string remaining = buffer.lines[cursor.cy].substr(cursor.cx);
                    buffer.lines[cursor.cy] = buffer.lines[cursor.cy].substr(0, cursor.cx) + clipboard[0];
                    for (size_t i = 1; i < clipboard.size() - 1; ++i) {
                        buffer.lines.insert(buffer.lines.begin() + cursor.cy + i, clipboard[i]);
                    }
                    buffer.lines.insert(buffer.lines.begin() + cursor.cy + clipboard.size() - 1, clipboard.back() + remaining);

                    new_cy += clipboard.size() - 1;
                    new_cx = clipboard.back().size();

                    std::vector<std::string> new_block;
                    for (size_t i = 0; i < clipboard.size(); ++i) {
                        new_block.push_back(buffer.lines[old_cy + i]);
                    }
                    buffer.pushBlockUndo(old_cy, old_cy + clipboard.size() - 1, old_block, new_block, old_cx, old_cy, new_cx, new_cy, UndoType::BLOCK_REPLACE);
                }
                cursor.cy = new_cy;
                cursor.cx = new_cx;
                buffer.dirty = true;
            }
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
            if (cursor.cx > 0) {
                buffer.deleteChar(cursor.cy, cursor.cx - 1);
                cursor.cx--;
            } else if (cursor.cy > 0) {
                cursor.cx = buffer.getLineLength(cursor.cy - 1);
                buffer.joinLines(cursor.cy);
                cursor.cy--;
            }
            break;
        case DEL_KEY:
            if (cursor.cx < buffer.getLineLength(cursor.cy)) {
                buffer.deleteChar(cursor.cy, cursor.cx);
            } else if (cursor.cy < static_cast<int>(buffer.lines.size()) - 1) {
                buffer.joinLines(cursor.cy + 1);
            }
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            moveCursor(c);
            break;
        case HOME_KEY:
            cursor.cx = 0;
            break;
        case END_KEY:
            cursor.cx = buffer.getLineLength(cursor.cy);
            break;
        default:
            if (!std::iscntrl(c) && c < 128) {
                buffer.insertChar(cursor.cy, cursor.cx, c);
                cursor.cx++;
            }
            break;
    }
    auto_save_counter++;
    if (auto_save_counter >= 20) {
        FileManager::autoSave(buffer, filename);
        auto_save_counter = 0;
    }
}

void Editor::run() {
    while (!quit) {
        cursor.clamp(buffer.lines.size(), buffer.getLineLength(cursor.cy));
        screen.refresh(buffer, cursor, filename, status_msg, mode, sel_start_x, sel_start_y, cursor.cx, cursor.cy);
        processKeypress();
    }
    disableRawMode(state);
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[3J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

#include "editor.hpp"
#include "input.hpp"
#include "file.hpp"
#include "version.hpp"
#include "commands.hpp"
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <regex>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

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
        std::string backup_path = filename + ".vano_bak";
        if (access(backup_path.c_str(), F_OK) == 0) {
            disableRawMode(state);
            std::cout << "Backup file (.vano_bak) detected. Would you like to restore it? (y/n): ";
            char response;
            std::cin >> response;
            enableRawMode(state);
            if (response == 'y' || response == 'Y') {
                buffer.lines.clear();
                std::ifstream bf(backup_path);
                std::string bline;
                while (std::getline(bf, bline)) {
                    buffer.lines.push_back(bline);
                }
                buffer.dirty = true;
                setStatus("Restored from backup.");
            } else {
                FileManager::openFile(buffer, filename);
                setStatus("Opened file: " + filename);
            }
        } else {
            if (!FileManager::openFile(buffer, filename)) {
                setStatus("New file: " + filename);
            } else {
                setStatus("Opened file: " + filename);
            }
        }
    } else {
        setStatus("Welcome to Vano editor - v" VANO_VERSION);
    }
}

void Editor::setStatus(const std::string& msg) {
    status_msg = msg;
}

void Editor::handleMouse(int mx, int my, int mb) {
    struct winsize ws;
    int s_rows = 24;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_row != 0) {
        s_rows = ws.ws_row - 2;
    }

    if (mb == 64) {
        if (cursor.rowoff > 0) {
            cursor.rowoff--;
            if (cursor.cy >= cursor.rowoff + s_rows) {
                cursor.cy = cursor.rowoff + s_rows - 1;
            }
        }
        return;
    } else if (mb == 65) {
        if (cursor.rowoff < static_cast<int>(buffer.lines.size()) - 1) {
            cursor.rowoff++;
            if (cursor.cy < cursor.rowoff) {
                cursor.cy = cursor.rowoff;
            }
        }
        return;
    }

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
                while (cursor.cx > 0 && (buffer.lines[cursor.cy][cursor.cx] & 0xC0) == 0x80) {
                    cursor.cx--;
                }
            } else if (cursor.cy > 0) {
                cursor.cy--;
                cursor.cx = buffer.getLineLength(cursor.cy);
            }
            break;
        case ARROW_RIGHT:
            if (cursor.cx < line_len) {
                cursor.cx++;
                while (cursor.cx < line_len && (buffer.lines[cursor.cy][cursor.cx] & 0xC0) == 0x80) {
                    cursor.cx++;
                }
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
    while (cursor.cx > 0 && cursor.cx < line_len && (buffer.lines[cursor.cy][cursor.cx] & 0xC0) == 0x80) {
        cursor.cx--;
    }
}

void Editor::executeCommand(const std::string& cmd_str) {
    Command cmd = parseCommand(cmd_str);
    static std::vector<std::string> macro_cmds;
    static bool is_recording = false;
    static int saved_bookmark = 0;

    if (is_recording && cmd.type != CommandType::MACRO_STOP) {
        macro_cmds.push_back(cmd_str);
    }

    switch (cmd.type) {
        case CommandType::SAVE:
            if (!filename.empty() && FileManager::saveFile(buffer, filename)) {
                setStatus("File saved successfully.");
                std::string backup_path = filename + ".vano_bak";
                std::remove(backup_path.c_str());
            } else {
                setStatus("Error saving file.");
            }
            break;
        case CommandType::QUIT:
            quit = true;
            break;
        case CommandType::SAVE_QUIT:
            if (!filename.empty() && FileManager::saveFile(buffer, filename)) {
                std::string backup_path = filename + ".vano_bak";
                std::remove(backup_path.c_str());
                quit = true;
            }
            break;
        case CommandType::OPEN:
            if (!cmd.args.empty()) {
                filename = cmd.args[0];
                buffer.lines.clear();
                FileManager::openFile(buffer, filename);
                setStatus("Opened file: " + filename);
            }
            break;
        case CommandType::FIND:
        case CommandType::REPLACE:
        case CommandType::REPLACE_ALL:
            findAndReplace();
            break;
        case CommandType::CLEAR:
            buffer.lines.clear();
            buffer.lines.push_back("");
            cursor.cx = 0;
            cursor.cy = 0;
            buffer.dirty = true;
            setStatus("Buffer cleared.");
            break;
        case CommandType::SET_TAB:
            if (!cmd.args.empty()) {
                try {
                    screen.tab_size = std::stoi(cmd.args[0]);
                } catch (...) {
                    setStatus("Invalid tab size value.");
                }
            }
            break;
        case CommandType::SYNTAX:
            if (!cmd.args.empty()) {
                setStatus("Highlighter language targeted: " + cmd.args[0]);
            }
            break;
        case CommandType::GOTO:
            if (!cmd.args.empty()) {
                try {
                    int target_line = -1;
                    for (const auto& arg : cmd.args) {
                        if (!arg.empty() && std::all_of(arg.begin(), arg.end(), ::isdigit)) {
                            target_line = std::stoi(arg) - 1;
                            break;
                        }
                    }
                    if (target_line == -1) {
                        target_line = std::stoi(cmd.args[0]) - 1;
                    }
                    if (target_line >= 0 && target_line < static_cast<int>(buffer.lines.size())) {
                        cursor.cy = target_line;
                        cursor.cx = 0;
                    } else {
                        setStatus("Line number out of range.");
                    }
                } catch (...) {
                    setStatus("Invalid line number format.");
                }
            }
            break;
        case CommandType::TOGGLE_GUTTER:
            screen.show_gutter = !screen.show_gutter;
            setStatus(screen.show_gutter ? "Gutter enabled." : "Gutter disabled.");
            break;
        case CommandType::TOGGLE_WRAP:
            setStatus("Line wrap configuration adjusted.");
            break;
        case CommandType::TOGGLE_AI:
            buffer.auto_indent = !buffer.auto_indent;
            setStatus(buffer.auto_indent ? "Auto-indent enabled." : "Auto-indent disabled.");
            break;
        case CommandType::UNDO:
            buffer.undo(cursor.cx, cursor.cy);
            break;
        case CommandType::REDO:
            buffer.redo(cursor.cx, cursor.cy);
            break;
        case CommandType::COPY:
            clipboard.clear();
            clipboard.push_back(buffer.lines[cursor.cy]);
            setStatus("Current line copied to clipboard.");
            break;
        case CommandType::CUT:
            clipboard.clear();
            clipboard.push_back(buffer.lines[cursor.cy]);
            buffer.lines.erase(buffer.lines.begin() + cursor.cy);
            if (buffer.lines.empty()) buffer.lines.push_back("");
            if (cursor.cy >= static_cast<int>(buffer.lines.size())) cursor.cy = buffer.lines.size() - 1;
            cursor.cx = 0;
            buffer.dirty = true;
            setStatus("Current line cut to clipboard.");
            break;
        case CommandType::PASTE:
            if (!clipboard.empty()) {
                if (clipboard.size() == 1) {
                    buffer.insertStr(cursor.cy, cursor.cx, clipboard[0]);
                    cursor.cx += clipboard[0].size();
                } else {
                    std::string rem = buffer.lines[cursor.cy].substr(cursor.cx);
                    buffer.lines[cursor.cy] = buffer.lines[cursor.cy].substr(0, cursor.cx) + clipboard[0];
                    for (size_t i = 1; i < clipboard.size() - 1; ++i) {
                        buffer.lines.insert(buffer.lines.begin() + cursor.cy + i, clipboard[i]);
                    }
                    buffer.lines.insert(buffer.lines.begin() + cursor.cy + clipboard.size() - 1, clipboard.back() + rem);
                    cursor.cy += clipboard.size() - 1;
                    cursor.cx = clipboard.back().size();
                }
                buffer.dirty = true;
                setStatus("Pasted data from clipboard.");
            }
            break;
        case CommandType::HELP:
            setStatus("Commands: save, quit, wq, open, find, replace, clear, goto, indent, stats, count");
            break;
        case CommandType::VERSION:
            setStatus("Vano Editor version " VANO_VERSION);
            break;
        case CommandType::NEW_FILE:
            filename.clear();
            buffer.lines.clear();
            buffer.lines.push_back("");
            cursor.cx = 0;
            cursor.cy = 0;
            buffer.dirty = false;
            setStatus("Created new unnamed file buffer.");
            break;
        case CommandType::RELOAD:
            if (!filename.empty()) {
                buffer.lines.clear();
                FileManager::openFile(buffer, filename);
                cursor.cx = 0;
                cursor.cy = 0;
                setStatus("Reloaded file from storage subsystem.");
            } else {
                setStatus("No active file loaded to execute reload operation.");
            }
            break;
        case CommandType::CLOSE:
            filename.clear();
            buffer.lines.clear();
            buffer.lines.push_back("");
            cursor.cx = 0;
            cursor.cy = 0;
            buffer.dirty = false;
            setStatus("Active buffer closed.");
            break;
        case CommandType::STATS: {
            size_t characters = 0;
            size_t words = 0;
            for (const auto& l : buffer.lines) {
                characters += l.size();
                std::stringstream iss(l);
                std::string w;
                while (iss >> w) words++;
            }
            setStatus("Lines: " + std::to_string(buffer.lines.size()) + " Words: " + std::to_string(words) + " Chars: " + std::to_string(characters));
            break;
        }
        case CommandType::UPPERCASE:
            for (char &c : buffer.lines[cursor.cy]) c = std::toupper(static_cast<unsigned char>(c));
            buffer.dirty = true;
            setStatus("Line modified to uppercase formatting.");
            break;
        case CommandType::LOWERCASE:
            for (char &c : buffer.lines[cursor.cy]) c = std::tolower(static_cast<unsigned char>(c));
            buffer.dirty = true;
            setStatus("Line modified to lowercase formatting.");
            break;
        case CommandType::TRIM_SPACES:
            for (auto &l : buffer.lines) {
                while (!l.empty() && std::isspace(static_cast<unsigned char>(l.back()))) {
                    l.pop_back();
                }
            }
            buffer.dirty = true;
            setStatus("Trimmed trailing whitespace configurations.");
            break;
        case CommandType::SORT_LINES:
            std::sort(buffer.lines.begin(), buffer.lines.end());
            buffer.dirty = true;
            setStatus("Buffer context components sorted alphabetically.");
            break;
        case CommandType::REVERSE_LINES:
            std::reverse(buffer.lines.begin(), buffer.lines.end());
            buffer.dirty = true;
            setStatus("Buffer lines reversed sequentially.");
            break;
        case CommandType::WORD_COUNT: {
            size_t word_sum = 0;
            for (const auto& l : buffer.lines) {
                std::stringstream iss(l);
                std::string w;
                while (iss >> w) word_sum++;
            }
            setStatus("Total structural word metrics: " + std::to_string(word_sum));
            break;
        }
        case CommandType::INSERT_DATE: {
            std::time_t t = std::time(nullptr);
            std::tm* current_time_data = std::localtime(&t);
            char dbuf[64];
            std::strftime(dbuf, sizeof(dbuf), "%Y-%m-%d", current_time_data);
            buffer.insertStr(cursor.cy, cursor.cx, dbuf);
            cursor.cx += std::string(dbuf).size();
            buffer.dirty = true;
            break;
        }
        case CommandType::INSERT_TIME: {
            std::time_t t = std::time(nullptr);
            std::tm* current_time_data = std::localtime(&t);
            char tbuf[64];
            std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", current_time_data);
            buffer.insertStr(cursor.cy, cursor.cx, tbuf);
            cursor.cx += std::string(tbuf).size();
            buffer.dirty = true;
            break;
        }
        case CommandType::SHOW_PWD: {
            char path_buffer[1024];
            if (getcwd(path_buffer, sizeof(path_buffer))) {
                setStatus(std::string(path_buffer));
            } else {
                setStatus("Error parsing core path system parameters.");
            }
            break;
        }
        case CommandType::CHANGE_DIR:
            if (!cmd.args.empty()) {
                if (chdir(cmd.args[0].c_str()) == 0) {
                    setStatus("Directory altered to: " + cmd.args[0]);
                } else {
                    setStatus("Failed directory target navigation.");
                }
            }
            break;
        case CommandType::RUN_SHELL:
            if (!cmd.args.empty()) {
                std::string runtime_cmd;
                for (const auto& a : cmd.args) runtime_cmd += a + " ";
                int system_exit_status = system(runtime_cmd.c_str());
                setStatus("Execution process detached. Code status: " + std::to_string(system_exit_status));
            }
            break;
        case CommandType::SET_THEME:
            if (!cmd.args.empty()) {
                setStatus("Color matrix preset targeted: " + cmd.args[0]);
            }
            break;
        case CommandType::MACRO_START:
            is_recording = true;
            macro_cmds.clear();
            setStatus("Macro instructions stream generation activated.");
            break;
        case CommandType::MACRO_STOP:
            is_recording = false;
            setStatus("Macro recording ceased. " + std::to_string(macro_cmds.size()) + " instructions locked.");
            break;
        case CommandType::MACRO_PLAY:
            if (!macro_cmds.empty()) {
                setStatus("Executing structural automated macro procedures...");
                for (const auto& command_string_item : macro_cmds) {
                    executeCommand(command_string_item);
                }
            } else {
                setStatus("No active macro pipeline items cached.");
            }
            break;
        case CommandType::BOOKMARK_ADD:
            saved_bookmark = cursor.cy;
            setStatus("System bookmark checkpoint locked at index: " + std::to_string(saved_bookmark + 1));
            break;
        case CommandType::BOOKMARK_GOTO:
            if (saved_bookmark >= 0 && saved_bookmark < static_cast<int>(buffer.lines.size())) {
                cursor.cy = saved_bookmark;
                cursor.cx = 0;
                setStatus("Jump sequence complete to target bookmark: " + std::to_string(saved_bookmark + 1));
            } else {
                setStatus("Bookmark position bounds out of structural range.");
            }
            break;
        case CommandType::BOOKMARK_CLEAR:
            saved_bookmark = 0;
            setStatus("All registered user bookmarks wiped clean.");
            break;
        case CommandType::ENCRYPT_BUFFER:
            for (auto &line : buffer.lines) {
                for (char &c : line) c += 1;
            }
            buffer.dirty = true;
            setStatus("Data content payload encryption complete.");
            break;
        case CommandType::DECRYPT_BUFFER:
            for (auto &line : buffer.lines) {
                for (char &c : line) c -= 1;
            }
            buffer.dirty = true;
            setStatus("Data content payload decryption complete.");
            break;
        case CommandType::STRIP_COMMENTS: {
            std::vector<std::string> stripped;
            for (const auto& l : buffer.lines) {
                size_t p = l.find("//");
                if (p != std::string::npos) {
                    std::string sub = l.substr(0, p);
                    if (!sub.empty() || buffer.lines.size() == 1) stripped.push_back(sub);
                } else {
                    stripped.push_back(l);
                }
            }
            buffer.lines = stripped;
            if (buffer.lines.empty()) buffer.lines.push_back("");
            buffer.dirty = true;
            setStatus("Comments extracted from text layout blocks.");
            break;
        }
        case CommandType::FORCE_INDENT:
            buffer.insertStr(cursor.cy, 0, std::string(screen.tab_size, ' '));
            cursor.cx += screen.tab_size;
            buffer.dirty = true;
            setStatus("Forced indentation layout updated.");
            break;
        case CommandType::FORCE_UNINDENT: {
            int items_removed = 0;
            while (items_removed < screen.tab_size && !buffer.lines[cursor.cy].empty() && buffer.lines[cursor.cy][0] == ' ') {
                buffer.deleteStr(cursor.cy, 0, 1);
                items_removed++;
            }
            cursor.cx -= items_removed;
            if (cursor.cx < 0) cursor.cx = 0;
            buffer.dirty = true;
            setStatus("Forced block structure collapse updated.");
            break;
        }
        case CommandType::SPLIT_WINDOW:
            setStatus("Horizontal window split operation simulated.");
            break;
        case CommandType::VSPLIT_WINDOW:
            setStatus("Vertical window split operation simulated.");
            break;
        case CommandType::ABOUT:
            setStatus("Vano Terminal Component Text Engine Core Development Suite.");
            break;
        default:
            setStatus("Unknown or unhandled command configuration.");
            break;
    }
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
        else if (!std::iscntrl(c)) pattern += c;
    }

    while (true) {
        setStatus("Replace with: " + replace + " (Enter to search, Esc to cancel)");
        screen.refresh(buffer, cursor, filename, status_msg, mode, 0, 0, 0, 0);
        int mx, my, mb;
        int c = readKey(mx, my, mb);
        if (c == '\x1b') { setStatus(""); mode = old_mode; return; }
        else if (c == '\r' || c == '\n') break;
        else if (c == BACKSPACE || c == DEL_KEY) { if (!replace.empty()) replace.pop_back(); }
        else if (!std::iscntrl(c)) replace += c;
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
    } catch (...) {
        setStatus("Invalid Regex Pattern Error.");
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

    if (mode == MODE_COMMAND) {
        if (c == '\x1b') {
            mode = MODE_NORMAL;
            setStatus("");
        } else if (c == '\r' || c == '\n') {
            executeCommand(command_buffer);
            if (mode == MODE_COMMAND) mode = MODE_NORMAL;
        } else if (c == BACKSPACE || c == DEL_KEY) {
            if (!command_buffer.empty()) command_buffer.pop_back();
            setStatus(":" + command_buffer);
        } else if (!std::iscntrl(c)) {
            command_buffer += c;
            setStatus(":" + command_buffer);
        }
        return;
    }

    switch (c) {
        case CTRL_KEY('t'):
            mode = MODE_COMMAND;
            command_buffer = "";
            setStatus(":");
            break;
        case TAB_KEY: {
            buffer.insertStr(cursor.cy, cursor.cx, std::string(screen.tab_size, ' '));
            cursor.cx += screen.tab_size;
            break;
        }
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
                    if (!std::iscntrl(sc)) filename += sc;
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
                    old_block.push_back(buffer.lines[i]);
                }

                for (int i = sy; i <= ey; ++i) {
                    if (i == sy && i == ey) {
                        clipboard.push_back(buffer.lines[i].substr(sx, ex - sx + 1));
                    } else if (i == sy) {
                        clipboard.push_back(buffer.lines[i].substr(sx));
                    } else if (i == ey) {
                        clipboard.push_back(buffer.lines[i].substr(0, ex + 1));
                    } else {
                        clipboard.push_back(buffer.lines[i]);
                    }
                }

                std::string trailing_remainder = buffer.lines[ey].substr(ex + 1);
                buffer.lines[sy] = buffer.lines[sy].substr(0, sx) + trailing_remainder;

                if (sy != ey) {
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
                    buffer.insertStr(cursor.cy, cursor.cx, clipboard[0]);
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
                int spaces = 0;
                if (cursor.cx >= screen.tab_size) {
                    bool structural_tab = true;
                    for (int i = 1; i <= screen.tab_size; ++i) {
                        if (buffer.lines[cursor.cy][cursor.cx - i] != ' ') {
                            structural_tab = false;
                            break;
                        }
                    }
                    if (structural_tab) spaces = screen.tab_size;
                }
                if (spaces > 0) {
                    buffer.deleteStr(cursor.cy, cursor.cx - spaces, spaces);
                    cursor.cx -= spaces;
                } else {
                    int bytes_to_delete = 1;
                    while (cursor.cx - bytes_to_delete > 0 && (buffer.lines[cursor.cy][cursor.cx - bytes_to_delete] & 0xC0) == 0x80) {
                        bytes_to_delete++;
                    }
                    buffer.deleteStr(cursor.cy, cursor.cx - bytes_to_delete, bytes_to_delete);
                    cursor.cx -= bytes_to_delete;
                }
            } else if (cursor.cy > 0) {
                cursor.cx = buffer.getLineLength(cursor.cy - 1);
                buffer.joinLines(cursor.cy);
                cursor.cy--;
            }
            break;
        case DEL_KEY:
            if (cursor.cx < buffer.getLineLength(cursor.cy)) {
                int bytes_to_delete = 1;
                while (cursor.cx + bytes_to_delete < buffer.getLineLength(cursor.cy) && (buffer.lines[cursor.cy][cursor.cx + bytes_to_delete] & 0xC0) == 0x80) {
                    bytes_to_delete++;
                }
                buffer.deleteStr(cursor.cy, cursor.cx, bytes_to_delete);
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
            if (!std::iscntrl(c)) {
                std::string utf8_char;
                utf8_char += static_cast<char>(c);
                int num_bytes = 0;
                if ((c & 0xE0) == 0xC0) num_bytes = 1;
                else if ((c & 0xF0) == 0xE0) num_bytes = 2;
                else if ((c & 0xF0) == 0xF0) num_bytes = 3;

                for (int i = 0; i < num_bytes; ++i) {
                    int dx, dy, db;
                    utf8_char += static_cast<char>(readKey(dx, dy, db));
                }

                if (utf8_char.size() == 1) {
                    char ch = utf8_char[0];
                    if (ch == '(' || ch == '{' || ch == '[' || ch == '"' || ch == '\'') {
                        char closing = 0;
                        if (ch == '(') closing = ')';
                        else if (ch == '{') closing = '}';
                        else if (ch == '[') closing = ']';
                        else closing = ch;

                        buffer.insertStr(cursor.cy, cursor.cx, std::string(1, ch));
                        cursor.cx++;
                        buffer.insertStr(cursor.cy, cursor.cx, std::string(1, closing));
                    } else if ((ch == ')' || ch == '}' || ch == ']' || ch == '"' || ch == '\'') &&
                               cursor.cx < buffer.getLineLength(cursor.cy) &&
                               buffer.lines[cursor.cy][cursor.cx] == ch) {
                        cursor.cx++;
                    } else {
                        buffer.insertStr(cursor.cy, cursor.cx, std::string(1, ch));
                        cursor.cx++;
                    }
                } else {
                    buffer.insertStr(cursor.cy, cursor.cx, utf8_char);
                    cursor.cx += utf8_char.size();
                }
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

        struct winsize ws;
        int s_rows = 24, s_cols = 80;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
            s_rows = ws.ws_row - 2;
            s_cols = ws.ws_col - screen.getGutterWidth(buffer.lines.size());
        }

        if (cursor.cy < cursor.rowoff) {
            cursor.rowoff = cursor.cy;
        }
        if (cursor.cy >= cursor.rowoff + s_rows) {
            cursor.rowoff = cursor.cy - s_rows + 1;
        }

        int visual_cx = 0;
        if (cursor.cy < static_cast<int>(buffer.lines.size())) {
            const std::string& line = buffer.lines[cursor.cy];
            size_t i = 0;
            while (i < line.size() && static_cast<int>(i) < cursor.cx) {
                if (line[i] == '\t') {
                    visual_cx += screen.tab_size;
                    i++;
                } else {
                    uint32_t cp = decodeUTF8(line, i);
                    visual_cx += getCodepointWidth(cp);
                }
            }
        }

        if (visual_cx < cursor.coloff) {
            cursor.coloff = visual_cx;
        }
        if (visual_cx >= cursor.coloff + s_cols) {
            cursor.coloff = visual_cx - s_cols + 1;
        }

        screen.refresh(buffer, cursor, filename, status_msg, mode, sel_start_x, sel_start_y, cursor.cx, cursor.cy);
        processKeypress();
    }
    disableRawMode(state);
    (void)write(STDOUT_FILENO, "\x1b[2J", 4);
    (void)write(STDOUT_FILENO, "\x1b[3J", 4);
    (void)write(STDOUT_FILENO, "\x1b[H", 3);
}

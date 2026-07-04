#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "buffer.hpp"
#include "cursor.hpp"
#include "screen.hpp"
#include "utils.hpp"
#include <string>
#include <vector>

class Editor {
public:
    Editor();
    ~Editor() = default;

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor(Editor&&) = delete;
    Editor& operator=(Editor&&) = delete;

    void init(const std::string& file_path);
    void run();
    void setStatus(const std::string& msg);

private:
    Buffer buffer;
    Cursor cursor;
    Screen screen;
    TerminalState state;
    
    std::string filename;
    std::string status_msg;
    std::string command_buffer;
    bool quit;
    int auto_save_counter;
    
    EditorMode mode;
    int sel_start_x;
    int sel_start_y;
    std::vector<std::string> clipboard;

    void moveCursor(int key);
    void processKeypress();
    void findAndReplace();
    void handleMouse(int mx, int my, int mb);
    void executeCommand(const std::string& cmd_str);
    std::string getFileExtension() const;
};

#endif
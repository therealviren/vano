#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "buffer.hpp"
#include "cursor.hpp"
#include "screen.hpp"
#include "utils.hpp"
#include <string>

class Editor {
private:
    Buffer buffer;
    Cursor cursor;
    Screen screen;
    TerminalState state;
    std::string filename;
    std::string status_msg;
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
    std::string getFileExtension() const;
public:
    Editor();
    void init(const std::string& file_path);
    void run();
    void setStatus(const std::string& msg);
};

#endif

#ifndef FILE_HPP
#define FILE_HPP

#include "buffer.hpp"
#include <string>

struct Theme {
    std::string name;
    std::string bg_color;
    std::string fg_color;
    std::string keyword;
    std::string type;
    std::string number;
    std::string string;
    std::string comment;
    std::string operator_color;
    std::string gutter_bg;
    std::string gutter_fg;
    std::string status_normal;
    std::string status_visual;
    std::string status_search;
    std::string status_command;
    std::string selection;
    std::string bracket;
    std::string reset;
    Theme();
};

class FileManager {
public:
    static bool openFile(Buffer& buffer, const std::string& filename);
    static bool saveFile(Buffer& buffer, const std::string& filename);
    static void autoSave(const Buffer& buffer, const std::string& filename);
    static void loadConfig(int& tab_size, bool& auto_indent, bool& show_gutter);
    static void loadTheme(Theme& theme);
};

#endif

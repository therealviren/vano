#ifndef FILE_HPP
#define FILE_HPP

#include "buffer.hpp"
#include <string>

class FileManager {
public:
    static bool openFile(Buffer& buffer, const std::string& filename);
    static bool saveFile(Buffer& buffer, const std::string& filename);
    static void autoSave(const Buffer& buffer, const std::string& filename);
    static void loadConfig(int& tab_size, bool& auto_indent, bool& show_gutter);
};

#endif

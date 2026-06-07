#include "file.hpp"
#include <fstream>
#include <cstdlib>

bool FileManager::openFile(Buffer& buffer, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    buffer.lines.clear();
    std::string line;
    while (std::getline(file, file.is_open() ? line : line)) {
        if (file.eof() && line.empty() && !buffer.lines.empty()) break;
        buffer.lines.push_back(line);
    }
    if (buffer.lines.empty()) {
        buffer.lines.push_back("");
    }
    buffer.dirty = false;
    return true;
}

bool FileManager::saveFile(Buffer& buffer, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    for (size_t i = 0; i < buffer.lines.size(); ++i) {
        file << buffer.lines[i];
        if (i < buffer.lines.size() - 1) {
            file << "\n";
        }
    }
    buffer.dirty = false;
    return true;
}

void FileManager::autoSave(const Buffer& buffer, const std::string& filename) {
    if (!buffer.dirty || filename.empty()) return;
    std::string save_path = filename + ".vano_bak";
    std::ofstream file(save_path);
    if (!file.is_open()) return;
    for (const auto& line : buffer.lines) {
        file << line << "\n";
    }
}

void FileManager::loadConfig(int& tab_size, bool& auto_indent, bool& show_gutter) {
    const char* home = std::getenv("HOME");
    if (!home) return;
    std::string rcpath = std::string(home) + "/.vanorc";
    std::ifstream file(rcpath);
    if (!file.is_open()) return;
    std::string key;
    int val;
    while (file >> key >> val) {
        if (key == "tab_size") tab_size = val;
        else if (key == "auto_indent") auto_indent = (val != 0);
        else if (key == "show_gutter") show_gutter = (val != 0);
    }
}

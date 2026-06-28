#include "file.hpp"
#include <fstream>
#include <cstdlib>
#include <iterator>
#include <filesystem>
#include <system_error>
#include <string>

namespace fs = std::filesystem;

Theme::Theme() {
    bg_color = "";
    fg_color = "\x1b[m";
    keyword = "\x1b[32m";
    type = "\x1b[34m";
    number = "\x1b[36m";
    string = "\x1b[33m";
    comment = "\x1b[35m";
    operator_color = "\x1b[38;5;208m";
    gutter_bg = "";
    gutter_fg = "\x1b[90m";
    status_normal = "\x1b[44;37m";
    status_visual = "\x1b[45;37m";
    status_search = "\x1b[42;37m";
    status_command = "\x1b[46;30m";
    selection = "\x1b[7m";
    bracket = "\x1b[1;44m";
    reset = "\x1b[m";
}

static std::string hexToAnsi(const std::string& hex, bool is_bg) {
    if (hex.length() >= 7 && hex[0] == '#') {
        try {
            int r = std::stoi(hex.substr(1, 2), nullptr, 16);
            int g = std::stoi(hex.substr(3, 2), nullptr, 16);
            int b = std::stoi(hex.substr(5, 2), nullptr, 16);
            return "\x1b[" + std::to_string(is_bg ? 48 : 38) + ";2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
        } catch (...) {
        }
    }
    return "";
}

static std::string getVanoDir() {
    const char* home = std::getenv("HOME");
    if (!home) return "";

    std::string dir = std::string(home) + "/.local/share/vano";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

static std::string findThemeFile() {
    std::string dir = getVanoDir();
    if (dir.empty()) return "";

    std::error_code ec;
    if (!fs::exists(dir, ec)) return "";

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;

        const fs::path& p = entry.path();
        if (p.has_extension() && p.extension() == ".json") {
            return p.string();
        }
    }

    return "";
}

static std::string extractStr(const std::string& content, const std::string& key) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return "";

    size_t quote1 = content.find('"', colon);
    if (quote1 == std::string::npos) return "";

    size_t quote2 = content.find('"', quote1 + 1);
    if (quote2 == std::string::npos) return "";

    return content.substr(quote1 + 1, quote2 - quote1 - 1);
}

void FileManager::loadTheme(Theme& theme) {
    std::string path = findThemeFile();
    if (path.empty()) {
        theme.reset = "\x1b[m" + theme.bg_color + theme.fg_color;
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        theme.reset = "\x1b[m" + theme.bg_color + theme.fg_color;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    theme.name = extractStr(content, "Name");

    std::string bg = extractStr(content, "background-color");
    if (!bg.empty()) theme.bg_color = hexToAnsi(bg, true);

    std::string fg = extractStr(content, "text-color");
    if (!fg.empty()) theme.fg_color = hexToAnsi(fg, false);

    std::string kw = extractStr(content, "keyword-color");
    if (!kw.empty()) theme.keyword = hexToAnsi(kw, false);

    std::string ty = extractStr(content, "type-color");
    if (!ty.empty()) theme.type = hexToAnsi(ty, false);

    std::string num = extractStr(content, "number-color");
    if (!num.empty()) theme.number = hexToAnsi(num, false);

    std::string str = extractStr(content, "string-color");
    if (!str.empty()) theme.string = hexToAnsi(str, false);

    std::string com = extractStr(content, "comment-color");
    if (!com.empty()) theme.comment = hexToAnsi(com, false);

    std::string op = extractStr(content, "operator-color");
    if (!op.empty()) theme.operator_color = hexToAnsi(op, false);

    std::string gbg = extractStr(content, "gutter-bg");
    if (!gbg.empty()) theme.gutter_bg = hexToAnsi(gbg, true);

    std::string gfg = extractStr(content, "gutter-fg");
    if (!gfg.empty()) theme.gutter_fg = hexToAnsi(gfg, false);

    std::string sn = extractStr(content, "status-normal-bg");
    std::string snf = extractStr(content, "status-normal-fg");
    if (!sn.empty() && !snf.empty()) theme.status_normal = hexToAnsi(sn, true) + hexToAnsi(snf, false);
    else if (!sn.empty()) theme.status_normal = hexToAnsi(sn, true) + "\x1b[37m";

    std::string sv = extractStr(content, "status-visual-bg");
    std::string svf = extractStr(content, "status-visual-fg");
    if (!sv.empty() && !svf.empty()) theme.status_visual = hexToAnsi(sv, true) + hexToAnsi(svf, false);
    else if (!sv.empty()) theme.status_visual = hexToAnsi(sv, true) + "\x1b[37m";

    std::string ss = extractStr(content, "status-search-bg");
    std::string ssf = extractStr(content, "status-search-fg");
    if (!ss.empty() && !ssf.empty()) theme.status_search = hexToAnsi(ss, true) + hexToAnsi(ssf, false);
    else if (!ss.empty()) theme.status_search = hexToAnsi(ss, true) + "\x1b[37m";

    std::string sc = extractStr(content, "status-command-bg");
    std::string scf = extractStr(content, "status-command-fg");
    if (!sc.empty() && !scf.empty()) theme.status_command = hexToAnsi(sc, true) + hexToAnsi(scf, false);
    else if (!sc.empty()) theme.status_command = hexToAnsi(sc, true) + "\x1b[30m";

    std::string sel = extractStr(content, "selection-bg");
    if (!sel.empty()) theme.selection = hexToAnsi(sel, true);

    std::string brk = extractStr(content, "bracket-match-bg");
    if (!brk.empty()) theme.bracket = hexToAnsi(brk, true);

    theme.reset = "\x1b[m" + theme.bg_color + theme.fg_color;
}

bool FileManager::openFile(Buffer& buffer, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    buffer.lines.clear();
    std::string line;
    while (std::getline(file, line)) {
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
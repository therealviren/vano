#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>
#include <vector>
#include <sstream>

enum class CommandType {
    SAVE,
    QUIT,
    SAVE_QUIT,
    OPEN,
    FIND,
    REPLACE,
    CLEAR,
    SET_TAB,
    SYNTAX,
    GOTO,
    TOGGLE_GUTTER,
    TOGGLE_WRAP,
    TOGGLE_AI,
    UNDO,
    REDO,
    COPY,
    CUT,
    PASTE,
    HELP,
    VERSION,
    NEW_FILE,
    RELOAD,
    CLOSE,
    STATS,
    UPPERCASE,
    LOWERCASE,
    TRIM_SPACES,
    SORT_LINES,
    REVERSE_LINES,
    WORD_COUNT,
    INSERT_DATE,
    INSERT_TIME,
    SHOW_PWD,
    CHANGE_DIR,
    RUN_SHELL,
    SET_THEME,
    MACRO_START,
    MACRO_STOP,
    MACRO_PLAY,
    BOOKMARK_ADD,
    BOOKMARK_GOTO,
    BOOKMARK_CLEAR,
    REPLACE_ALL,
    ENCRYPT_BUFFER,
    DECRYPT_BUFFER,
    STRIP_COMMENTS,
    FORCE_INDENT,
    FORCE_UNINDENT,
    SPLIT_WINDOW,
    VSPLIT_WINDOW,
    ABOUT,
    UNKNOWN
};

struct Command {
    CommandType type;
    std::vector<std::string> args;
};

inline Command parseCommand(const std::string& input) {
    std::stringstream ss(input);
    std::string action;
    ss >> action;

    Command cmd;
    cmd.type = CommandType::UNKNOWN;

    if (action == "w" || action == "save") cmd.type = CommandType::SAVE;
    else if (action == "q" || action == "quit") cmd.type = CommandType::QUIT;
    else if (action == "wq" || action == "x") cmd.type = CommandType::SAVE_QUIT;
    else if (action == "open" || action == "o") cmd.type = CommandType::OPEN;
    else if (action == "find") cmd.type = CommandType::FIND;
    else if (action == "replace") cmd.type = CommandType::REPLACE;
    else if (action == "clear") cmd.type = CommandType::CLEAR;
    else if (action == "syntax") cmd.type = CommandType::SYNTAX;
    else if (action == "line" || action == "goto") cmd.type = CommandType::GOTO;
    else if (action == "undo" || action == "u") cmd.type = CommandType::UNDO;
    else if (action == "redo" || action == "r") cmd.type = CommandType::REDO;
    else if (action == "copy") cmd.type = CommandType::COPY;
    else if (action == "cut") cmd.type = CommandType::CUT;
    else if (action == "paste") cmd.type = CommandType::PASTE;
    else if (action == "help" || action == "h") cmd.type = CommandType::HELP;
    else if (action == "version" || action == "v") cmd.type = CommandType::VERSION;
    else if (action == "new") cmd.type = CommandType::NEW_FILE;
    else if (action == "reload") cmd.type = CommandType::RELOAD;
    else if (action == "close") cmd.type = CommandType::CLOSE;
    else if (action == "stats") cmd.type = CommandType::STATS;
    else if (action == "upper") cmd.type = CommandType::UPPERCASE;
    else if (action == "lower") cmd.type = CommandType::LOWERCASE;
    else if (action == "trim") cmd.type = CommandType::TRIM_SPACES;
    else if (action == "sort") cmd.type = CommandType::SORT_LINES;
    else if (action == "reverse") cmd.type = CommandType::REVERSE_LINES;
    else if (action == "count") cmd.type = CommandType::WORD_COUNT;
    else if (action == "date") cmd.type = CommandType::INSERT_DATE;
    else if (action == "time") cmd.type = CommandType::INSERT_TIME;
    else if (action == "pwd") cmd.type = CommandType::SHOW_PWD;
    else if (action == "cd") cmd.type = CommandType::CHANGE_DIR;
    else if (action == "shell" || action == "run") cmd.type = CommandType::RUN_SHELL;
    else if (action == "theme") cmd.type = CommandType::SET_THEME;
    else if (action == "replaceall") cmd.type = CommandType::REPLACE_ALL;
    else if (action == "encrypt") cmd.type = CommandType::ENCRYPT_BUFFER;
    else if (action == "decrypt") cmd.type = CommandType::DECRYPT_BUFFER;
    else if (action == "strip") cmd.type = CommandType::STRIP_COMMENTS;
    else if (action == "indent") cmd.type = CommandType::FORCE_INDENT;
    else if (action == "unindent") cmd.type = CommandType::FORCE_UNINDENT;
    else if (action == "split") cmd.type = CommandType::SPLIT_WINDOW;
    else if (action == "vsplit") cmd.type = CommandType::VSPLIT_WINDOW;
    else if (action == "about") cmd.type = CommandType::ABOUT;
    else if (action == "set") {
        std::string sub;
        ss >> sub;
        if (sub == "tab") cmd.type = CommandType::SET_TAB;
        else if (sub == "gutter") cmd.type = CommandType::TOGGLE_GUTTER;
        else if (sub == "wrap") cmd.type = CommandType::TOGGLE_WRAP;
        else if (sub == "ai") cmd.type = CommandType::TOGGLE_AI;
    }
    else if (action == "macro") {
        std::string sub;
        ss >> sub;
        if (sub == "start") cmd.type = CommandType::MACRO_START;
        else if (sub == "stop") cmd.type = CommandType::MACRO_STOP;
        else if (sub == "play") cmd.type = CommandType::MACRO_PLAY;
    }
    else if (action == "bookmark") {
        std::string sub;
        ss >> sub;
        if (sub == "add") cmd.type = CommandType::BOOKMARK_ADD;
        else if (sub == "go") cmd.type = CommandType::BOOKMARK_GOTO;
        else if (sub == "clear") cmd.type = CommandType::BOOKMARK_CLEAR;
    }

    std::string argument;
    while (ss >> argument) {
        cmd.args.push_back(argument);
    }

    return cmd;
}

#endif

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
    else if (action == "wq") cmd.type = CommandType::SAVE_QUIT;
    else if (action == "open") cmd.type = CommandType::OPEN;
    else if (action == "find") cmd.type = CommandType::FIND;
    else if (action == "replace") cmd.type = CommandType::REPLACE;
    else if (action == "clear") cmd.type = CommandType::CLEAR;
    else if (action == "syntax") cmd.type = CommandType::SYNTAX;
    else if (action == "line" || action == "goto") cmd.type = CommandType::GOTO;
    else if (action == "set") {
        std::string sub;
        ss >> sub;
        if (sub == "tab") {
            cmd.type = CommandType::SET_TAB;
        }
    }

    std::string argument;
    while (ss >> argument) {
        cmd.args.push_back(argument);
    }

    return cmd;
}

#endif

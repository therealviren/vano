#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <string>
#include <vector>
#include <memory>

enum class UndoType {
    CHAR_EDIT,
    NEWLINE,
    JOIN,
    BLOCK_REPLACE
};

struct UndoNode {
    bool is_insert;
    int x;
    int y;
    std::string text;
    std::vector<std::shared_ptr<UndoNode>> children;
    std::weak_ptr<UndoNode> parent;

    UndoType type = UndoType::CHAR_EDIT;
    std::vector<std::string> old_block;
    std::vector<std::string> new_block;
    int start_y = 0;
    int end_y = 0;
    int old_cx = 0;
    int old_cy = 0;
    int new_cx = 0;
    int new_cy = 0;
};

class Buffer {
public:
    std::vector<std::string> lines;
    std::shared_ptr<UndoNode> current_node;
    std::shared_ptr<UndoNode> root_node;
    bool dirty;
    bool auto_indent;

    Buffer();
    void insertChar(int y, int x, char c);
    void deleteChar(int y, int x);
    void insertStr(int y, int x, const std::string& s);
    void deleteStr(int y, int x, int len);
    void insertNewline(int y, int x);
    void joinLines(int y);
    int getLineLength(int y) const;
    void pushUndo(bool is_insert, int x, int y, const std::string& text);
    void pushBlockUndo(int start_y, int end_y, const std::vector<std::string>& old_block, const std::vector<std::string>& new_block, int old_cx, int old_cy, int new_cx, int new_cy, UndoType type);
    void undo(int& cx, int& cy);
    void redo(int& cx, int& cy);
    void toggleComment(int start_y, int end_y, const std::string& ext);
};

#endif

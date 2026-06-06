#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <string>
#include <vector>
#include <memory>

struct UndoNode {
    bool is_insert;
    int x;
    int y;
    std::string text;
    std::vector<std::shared_ptr<UndoNode>> children;
    std::shared_ptr<UndoNode> parent;
};

class Buffer {
public:
    std::vector<std::string> lines;
    std::shared_ptr<UndoNode> current_node;
    bool dirty;
    bool auto_indent;

    Buffer();
    void insertChar(int y, int x, char c);
    void deleteChar(int y, int x);
    void insertNewline(int y, int x);
    void joinLines(int y);
    int getLineLength(int y) const;
    void pushUndo(bool is_insert, int x, int y, const std::string& text);
    void undo(int& cx, int& cy);
    void redo(int& cx, int& cy);
    void toggleComment(int start_y, int end_y, const std::string& ext);
};

#endif

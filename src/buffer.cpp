#include "buffer.hpp"

Buffer::Buffer() : dirty(false), auto_indent(true) {
    lines.push_back("");
    current_node = std::make_shared<UndoNode>();
}

void Buffer::insertChar(int y, int x, char c) {
    if (y < 0 || y >= static_cast<int>(lines.size())) return;
    if (x < 0 || x > static_cast<int>(lines[y].size())) x = lines[y].size();
    pushUndo(true, x, y, std::string(1, c));
    lines[y].insert(x, 1, c);
    dirty = true;
}

void Buffer::deleteChar(int y, int x) {
    if (y < 0 || y >= static_cast<int>(lines.size())) return;
    if (x < 0 || x >= static_cast<int>(lines[y].size())) return;
    pushUndo(false, x, y, std::string(1, lines[y][x]));
    lines[y].erase(x, 1);
    dirty = true;
}

void Buffer::insertNewline(int y, int x) {
    if (y < 0 || y >= static_cast<int>(lines.size())) return;
    if (x < 0 || x > static_cast<int>(lines[y].size())) x = lines[y].size();
    std::string split = lines[y].substr(x);
    lines[y] = lines[y].substr(0, x);
    
    std::string indent = "";
    if (auto_indent) {
        for (char c : lines[y]) {
            if (c == ' ' || c == '\t') indent += c;
            else break;
        }
    }
    
    lines.insert(lines.begin() + y + 1, indent + split);
    dirty = true;
}

void Buffer::joinLines(int y) {
    if (y <= 0 || y >= static_cast<int>(lines.size())) return;
    lines[y - 1] += lines[y];
    lines.erase(lines.begin() + y);
    dirty = true;
}

int Buffer::getLineLength(int y) const {
    if (y < 0 || y >= static_cast<int>(lines.size())) return 0;
    return lines[y].size();
}

void Buffer::pushUndo(bool is_insert, int x, int y, const std::string& text) {
    auto node = std::make_shared<UndoNode>();
    node->is_insert = is_insert;
    node->x = x;
    node->y = y;
    node->text = text;
    node->parent = current_node;
    current_node->children.push_back(node);
    current_node = node;
}

void Buffer::undo(int& cx, int& cy) {
    if (!current_node || !current_node->parent) return;
    if (current_node->is_insert) {
        if (current_node->y >= 0 && current_node->y < static_cast<int>(lines.size())) {
            lines[current_node->y].erase(current_node->x, current_node->text.size());
            cx = current_node->x;
            cy = current_node->y;
        }
    } else {
        if (current_node->y >= 0 && current_node->y < static_cast<int>(lines.size())) {
            lines[current_node->y].insert(current_node->x, current_node->text);
            cx = current_node->x + current_node->text.size();
            cy = current_node->y;
        }
    }
    current_node = current_node->parent;
}

void Buffer::redo(int& cx, int& cy) {
    if (current_node->children.empty()) return;
    auto next_node = current_node->children.back();
    if (next_node->is_insert) {
        if (next_node->y >= 0 && next_node->y < static_cast<int>(lines.size())) {
            lines[next_node->y].insert(next_node->x, next_node->text);
            cx = next_node->x + next_node->text.size();
            cy = next_node->y;
        }
    } else {
        if (next_node->y >= 0 && next_node->y < static_cast<int>(lines.size())) {
            lines[next_node->y].erase(next_node->x, next_node->text.size());
            cx = next_node->x;
            cy = next_node->y;
        }
    }
    current_node = next_node;
}

void Buffer::toggleComment(int start_y, int end_y, const std::string& ext) {
    std::string prefix = "// ";
    if (ext == ".py" || ext == ".sh" || ext == ".rb") prefix = "# ";
    else if (ext == ".lua") prefix = "-- ";

    if (start_y > end_y) std::swap(start_y, end_y);
    for (int i = start_y; i <= end_y; ++i) {
        if (i < 0 || i >= static_cast<int>(lines.size())) continue;
        if (lines[i].rfind(prefix, 0) == 0) {
            lines[i].erase(0, prefix.size());
        } else {
            lines[i].insert(0, prefix);
        }
    }
    dirty = true;
}

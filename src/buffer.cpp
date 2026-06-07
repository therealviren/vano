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

    std::vector<std::string> old_block = { lines[y] };
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

    std::vector<std::string> new_block = { lines[y], lines[y + 1] };
    pushBlockUndo(y, y + 1, old_block, new_block, x, y, 0, y + 1, UndoType::NEWLINE);
    dirty = true;
}

void Buffer::joinLines(int y) {
    if (y <= 0 || y >= static_cast<int>(lines.size())) return;
    std::vector<std::string> old_block = { lines[y - 1], lines[y] };
    int prev_len = lines[y - 1].size();

    lines[y - 1] += lines[y];
    lines.erase(lines.begin() + y);

    std::vector<std::string> new_block = { lines[y - 1] };
    pushBlockUndo(y - 1, y, old_block, new_block, prev_len, y - 1, prev_len, y - 1, UndoType::JOIN);
    dirty = true;
}

int Buffer::getLineLength(int y) const {
    if (y < 0 || y >= static_cast<int>(lines.size())) return 0;
    return lines[y].size();
}

void Buffer::pushUndo(bool is_insert, int x, int y, const std::string& text) {
    if (!current_node->children.empty()) {
        current_node->children.clear();
    }
    auto node = std::make_shared<UndoNode>();
    node->is_insert = is_insert;
    node->x = x;
    node->y = y;
    node->text = text;
    node->type = UndoType::CHAR_EDIT;
    node->parent = current_node;
    current_node->children.push_back(node);
    current_node = node;
}

void Buffer::pushBlockUndo(int start_y, int end_y, const std::vector<std::string>& old_block, const std::vector<std::string>& new_block, int old_cx, int old_cy, int new_cx, int new_cy, UndoType type) {
    if (!current_node->children.empty()) {
        current_node->children.clear();
    }
    auto node = std::make_shared<UndoNode>();
    node->type = type;
    node->start_y = start_y;
    node->end_y = end_y;
    node->old_block = old_block;
    node->new_block = new_block;
    node->old_cx = old_cx;
    node->old_cy = old_cy;
    node->new_cx = new_cx;
    node->new_cy = new_cy;
    node->parent = current_node;
    current_node->children.push_back(node);
    current_node = node;
}

void Buffer::undo(int& cx, int& cy) {
    if (!current_node) return;
    auto parent_ptr = current_node->parent.lock();
    if (!parent_ptr) return;

    if (current_node->type == UndoType::CHAR_EDIT) {
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
    } else {
        int num_to_remove = current_node->new_block.size();
        if (current_node->start_y >= 0 && current_node->start_y + num_to_remove <= static_cast<int>(lines.size())) {
            lines.erase(lines.begin() + current_node->start_y, lines.begin() + current_node->start_y + num_to_remove);
            lines.insert(lines.begin() + current_node->start_y, current_node->old_block.begin(), current_node->old_block.end());
            cx = current_node->old_cx;
            cy = current_node->old_cy;
        }
    }
    current_node = parent_ptr;
}

void Buffer::redo(int& cx, int& cy) {
    if (current_node->children.empty()) return;
    auto next_node = current_node->children.back();

    if (next_node->type == UndoType::CHAR_EDIT) {
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
    } else {
        int num_to_remove = next_node->old_block.size();
        if (next_node->start_y >= 0 && next_node->start_y + num_to_remove <= static_cast<int>(lines.size())) {
            lines.erase(lines.begin() + next_node->start_y, lines.begin() + next_node->start_y + num_to_remove);
            lines.insert(lines.begin() + next_node->start_y, next_node->new_block.begin(), next_node->new_block.end());
            cx = next_node->new_cx;
            cy = next_node->new_cy;
        }
    }
    current_node = next_node;
}

void Buffer::toggleComment(int start_y, int end_y, const std::string& ext) {
    std::string prefix = "// ";
    if (ext == ".py" || ext == ".sh" || ext == ".rb") prefix = "# ";
    else if (ext == ".lua") prefix = "-- ";

    if (start_y > end_y) std::swap(start_y, end_y);

    std::vector<std::string> old_block;
    for (int i = start_y; i <= end_y; ++i) {
        if (i >= 0 && i < static_cast<int>(lines.size())) {
            old_block.push_back(lines[i]);
        }
    }

    for (int i = start_y; i <= end_y; ++i) {
        if (i < 0 || i >= static_cast<int>(lines.size())) continue;
        if (lines[i].rfind(prefix, 0) == 0) {
            lines[i].erase(0, prefix.size());
        } else {
            lines[i].insert(0, prefix);
        }
    }

    std::vector<std::string> new_block;
    for (int i = start_y; i <= end_y; ++i) {
        if (i >= 0 && i < static_cast<int>(lines.size())) {
            new_block.push_back(lines[i]);
        }
    }

    pushBlockUndo(start_y, end_y, old_block, new_block, 0, start_y, 0, start_y, UndoType::BLOCK_REPLACE);
    dirty = true;
}

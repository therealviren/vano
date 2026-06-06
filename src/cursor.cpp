#include "cursor.hpp"

Cursor::Cursor() : cx(0), cy(0), rx(0), rowoff(0), coloff(0) {}

void Cursor::clamp(int num_lines, int line_len) {
    if (cy < 0) cy = 0;
    if (cy >= num_lines && num_lines > 0) cy = num_lines - 1;
    if (cx < 0) cx = 0;
    if (cx > line_len) cx = line_len;
}

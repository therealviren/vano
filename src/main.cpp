#include "editor.hpp"

int main(int argc, char* argv[]) {
    Editor editor;
    std::string path = "";
    if (argc > 1) {
        path = argv[1];
    }
    editor.init(path);
    editor.run();
    return 0;
}

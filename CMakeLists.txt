cmake_minimum_required(VERSION 3.12)
project(vano VERSION 1.4.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vano
    src/main.cpp
    src/editor.cpp
    src/buffer.cpp
    src/cursor.cpp
    src/screen.cpp
    src/input.cpp
    src/file.cpp
    src/utils.cpp
)

target_include_directories(vano PRIVATE src)

install(TARGETS vano DESTINATION bin)

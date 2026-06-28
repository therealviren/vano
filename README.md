# Vano

[![License](https://img.shields.io/github/license/therealviren/vano)](LICENSE)

Fast, dependency-free modal terminal text editor written in modern C++17.

Vano is built for speed, simplicity, and a clean terminal editing workflow. It combines the minimal feel of classic terminal editors with practical features like visual selection, regex search and replace, crash recovery, mouse support, smart indentation, and smooth horizontal scrolling.

## Features

- Modal editing workflow
- Visual mode with copy, cut, and paste
- Regular expression search and replace
- Smart bracket, brace, and parenthesis matching
- Crash recovery with automatic backup snapshots
- Command bar for quick editor actions
- Horizontal scrolling for long lines
- Smart backspace handling for tab indentation
- Flicker-free double buffered rendering
- Live cursor position tracking
- Configuration support through `~/.vanorc`
- Mouse support for cursor positioning and selection

## Installation

### Using the installer script

```bash
chmod +x install.sh
./install.sh
```

The installer will:

- detect a supported compiler
- install required dependencies
- build the project
- place the binary in your `PATH`

### Using CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

Open a file:

```bash
vano file.txt
```

Start with a new buffer:

```bash
vano
```


## Configuration

Vano reads configuration from:

```text
~/.vanorc
```

Example:

```text
tab_size 4
auto_indent 1
show_gutter 1
```

### Supported Options

- `tab_size` — tab width in spaces
- `auto_indent` — enable automatic indentation
- `show_gutter` — show line numbers in the gutter

## Command Bar

Open the command bar with:

```text
Ctrl + T
```

Available commands:

```text
w
q
wq
open <file>
goto <line>
set tab <size>
```

Example:

```text
open src/main.cpp
goto 120
set tab 4
wq
```

## Recovery

Vano automatically writes recovery snapshots to:

```text
.vano_bak
```

If the editor exits unexpectedly, Vano will prompt you to restore the previous session on the next startup.

## Keybindings

| Shortcut | Mode | Description |
| :--- | :--- | :--- |
| `Ctrl + Q` | Global | Quit editor |
| `Ctrl + S` | Global | Save file |
| `Ctrl + T` | Normal | Open command bar |
| `Ctrl + F` | Global | Regex search |
| `Ctrl + \` | Global | Regex replace |
| `Ctrl + /` | Global | Toggle comment |
| `Ctrl + Z` | Global | Undo |
| `Ctrl + R` | Global | Redo |
| `Ctrl + V` | Normal | Toggle visual mode |
| `Ctrl + C` | Visual | Copy selection |
| `Ctrl + X` | Visual | Cut selection |
| `Ctrl + P` | Normal | Paste clipboard |
| `Home` | Global | Move to start of line |
| `End` | Global | Move to end of line |

## Architecture

Vano is split into focused components:

| Component | Description |
| :--- | :--- |
| `Buffer` | Text storage, editing operations, undo/redo management |
| `Screen` | Rendering, scrolling, gutters, and status UI |
| `Editor` | Event loop, input processing, modes, and commands |
| `FileManager` | File I/O, configuration loading, and recovery handling |

## Requirements

- C++17 compatible compiler
- POSIX-compatible terminal
- CMake 3.10+ (optional)

## Contributing

Pull requests and issues are welcome.

For significant changes, please open an issue first to discuss the proposed implementation.

## Troubleshooting

If something is not working as expected, verify:

- your terminal supports ANSI escape sequences
- your compiler supports C++17
- your `.vanorc` configuration is valid
- the recovery file `.vano_bak` is not corrupted

## License

Released under the MIT License.

---

Made with :heart: by the Vano team.
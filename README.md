# Vano

Vano is a fast, dependency-free modal terminal text editor written in modern C++17. Designed for simplicity and performance, it combines the minimalist nature of classic terminal editors with modern quality-of-life features like visual text selection, regex operations, and mouse support.

---

## Key Features

* **Visual Mode & Clipboard:** Seamlessly select text blocks with inverted terminal rendering, then cut, copy, or paste lines effortlessly.
* **Regex Search & Replace:** Execute advanced text modifications using precise regular expression matching with an interactive confirmation workflow.
* **Smart Bracket Matching:** Real-time visual highlighting of matching brackets, braces, and parentheses to maintain structural awareness.
* **Crash Recovery & Auto-Save:** Background tracking engine automatically writes state snapshots to a recovery backup file (`.vano_bak`) and prompts for interactive restoration upon startup if an unexpected termination is detected.
* **Command Bar Mode (Ctrl + T):** Built-in micro-command line parsing interface allowing standard operations like file saving, quitting, loading resources, modifying configurations, or jumping to rows.
* **Horizontal Scrolling:** Smooth horizontal screen movement that dynamically adapts as the cursor passes the horizontal margins, preventing long source lines from being clipped out of view.
* **Smart Backspace for Tabs:** Intelligent deletion logic that recognizes block indentation spaces; pressing backspace wipes an entire tab-width of spaces at once instead of a single character.
* **Flicker-Free Double Buffering:** High-performance rendering pipeline changes that buffer terminal graphics, eliminating screen flicker during rapid text entry or large file navigation.
* **Live Coordinate Tracking:** Real-time visibility of exact cursor positions (`Ln X, Col Y`) displayed inside the right-hand status panel.
* **Configuration Support:** Custom preference definitions fetched directly via an optional run-control layout file.
* **Mouse Integration:** Support for standard terminal mouse interactions allowing on-click cursor adjustments.

---

## Technical Architecture

The core framework is systematically split into isolated components to ensure maintainability:

* `Buffer`: Manages data structures, lines tracking, undo/redo states, and structural text adjustments.
* `Screen`: Handles layout composition, scrolling bounds, line gutters, status messaging, and low-level escape sequence formatting.
* `Editor`: Controls operational states, execution loops, user interaction, selection math, and key binding logic.
* `FileManager`: Evaluates external asset input/output paths and extracts preference data mappings.

---

## Installation

### Method 1: Using the Installer Script
The project includes an automatic deployment script that resolves system dependencies, detects compilers, builds the binary, and places it inside the executable path.

```bash
chmod +x install.sh
./install.sh
```

### Method 2: Building via CMake
For standardized platforms or build pipelines, compile using the native configuration files:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Configuration

You can personalize the editor by adding a configuration file named `.vanorc` directly in your home directory (`~/.vanorc`). 

Supported options accept integer attributes:

```text
tab_size 4
auto_indent 1
show_gutter 1
```

---

## Keybindings

| Shortcut | Mode | Description |
| :--- | :--- | :--- |
| `Ctrl + Q` | Global | Exit Editor (Prompts if changes are unsaved) |
| `Ctrl + S` | Global | Save File |
| `Ctrl + T` | Normal | Open Command Bar (Supports `w`, `q`, `wq`, `open <file>`, `set tab <size>`, `goto <line>`) |
| `Ctrl + F` / `Ctrl + \` | Global | Regular Expression Search and Replace |
| `Ctrl + /` | Global | Toggle Comment on Active Line or Visual Selection |
| `Ctrl + Z` | Global | Undo Action |
| `Ctrl + R` | Global | Redo Action |
| `Ctrl + V` | Normal | Toggle Visual Mode (Text Selection) |
| `Ctrl + C` | Visual | Copy Selected Text |
| `Ctrl + X` | Visual | Cut Selected Text |
| `Ctrl + P` | Normal | Paste Clipboard Contents |
| `Home` / `End` | Global | Move Cursor to Start or End of Current Line |

---

## Limitations & Current Scope

Vano currently targets single-byte standard ASCII/UTF-8 character lengths for cursor mapping computations. Files utilizing multi-byte characters or complex emoji structures may experience visual displacement relative to physical cursor tracking indices. This optimization vector is slated for a future milestone update.

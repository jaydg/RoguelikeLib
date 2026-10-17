# Getting Started with RTUI {#rtui_getting_started}

**RTUI (Roguelike Terminal UI)** is a standalone C++20 terminal user interface toolkit designed for roguelikes, games, and rich interactive console tools.

> [!NOTE]
> RTUI is an independent library located in the `rtui/` directory. It uses POSIX terminal APIs for raw input, mouse reporting, alternate screen buffers, and provides diff-based screen compositing, a docking window manager, and widgets.

---

## Target Integration with Xmake

RTUI depends on `RoguelikeLib` for basic geometric primitives (`RL::Position`, `RL::Size`, `RL::Rect`, and `RL::CMap` support). In your `xmake.lua`:

```lua
includes("path/to/RoguelikeLib/roguelikelib")
includes("path/to/RoguelikeLib/rtui")

target("MyTerminalApp")
    set_kind("binary")
    set_languages("c++20")
    add_deps("RTUI") -- Publicly pulls in RoguelikeLib
    add_files("src/**.cpp")
```

---

## C++20 Module Structure

RTUI components are exported as C++20 modules under the `rtui.*` hierarchy:

| Module | Purpose | Key Symbols |
| :--- | :--- | :--- |
| `rtui.terminal` | POSIX terminal lifecycle and raw input | `RL::CTerminal`, `RL::SKey` |
| `rtui.screen` | Screen buffer and diff-based compositing | `RL::CScreen` |
| `rtui.windows` | Docking window manager and window base | `RL::CWindowManager`, `RL::CWindow`, `RL::EWindowOptions` |
| `rtui.list` | Interactive selectable list window widget | `RL::CListWindow` |
| `rtui.input` | Interactive text entry window widget | `RL::CInputWindow` |
| `rtui.map` | Map viewport drawing and camera math | `RL::DrawMap()`, `RL::CameraOrigin()`, `RL::OnScreen()` |
| `rtui.mouse` | Mouse tracking and coordinate decoders | `RL::SMouseEvent` |
| `rtui.utf8` | UTF-8 encoding and decoding helpers | `RL::Utf8Length()`, `RL::Utf8CharAt()` |
| `stc` | Terminal true-color and ANSI styles | Color escape helpers in `rtui/extern` |

---

## Minimal Hello World

Here is a complete interactive application that sets up the terminal, creates a screen buffer, prints a greeting, and waits for a keypress:

```cpp
import rtui.terminal;
import rtui.screen;
import std;

int main() {
    // 1. Enter raw terminal mode and switch to alternate screen
    RL::CTerminal terminal;

    // 2. Create a screen buffer sized to the terminal dimensions
    RL::CScreen screen(terminal.getSize());

    // 3. Print text (with optional colour markup)
    screen.Print(2, 2, "Hello from RTUI! Press any key to quit.");

    // 4. Compose and flush output
    terminal.Write(screen.Compose(true /* true_color */));

    // 5. Wait for keypress
    auto key = terminal.ReadKey();

    // CTerminal destructor automatically restores terminal state and screen!
    return 0;
}
```

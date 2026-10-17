# Terminal Lifecycle & Screen Compositing {#rtui_screen_terminal}

RTUI provides robust low-level terminal handling alongside an optimized differential screen compositing engine.

---

## The POSIX Terminal Layer (`RL::CTerminal`)

`RL::CTerminal` manages raw input mode, the alternate screen buffer, mouse tracking, and cursor visibility on POSIX systems:

- **Resource Ownership**: Only one `CTerminal` instance may be active at any time. Both `stdin` and `stdout` must be interactive terminals (isatty).
- **Graceful Restoration**: Automatically restores the user's terminal state, original screen, cursor visibility, and normal mode upon destruction, program exit, or unexpected termination signals (`SIGINT`, `SIGTERM`).
- **Input Decoding**: `terminal.ReadKey(timeout_ms)` handles Unicode keys, arrow keys, function keys, mouse drag/click events, terminal resize events (`SIGWINCH`), and window closures.
- **Offline Decoding**: `RL::DecodeKey(buffer)` can decode input packets directly from a memory buffer without requiring an active `CTerminal` instance.

---

## Differential Screen Compositing (`RL::CScreen`)

`RL::CScreen` maintains a 2D matrix of terminal cells (character, foreground, background, and attributes).

### Differential Frame Updates
Rather than redrawing the whole screen on every frame (which causes flickering and high bandwidth consumption over SSH), `CScreen::Compose` supports differential updates:

```cpp
std::string Compose(bool true_color = false,
                    const CScreen* previous_screen = nullptr) const;
```

- **Delta Output**: When passing `&previous_screen`, `Compose` emits ANSI escape sequences only for cells that changed since the last frame.
- **No-Op Frames**: If the current and previous screens are identical, `Compose` emits an empty string, keeping CPU and I/O usage minimal.
- **Resize Handling**: If the screen dimensions change between frames, `Compose` automatically triggers a complete screen redraw.
- **Feature Demo Performance**: The interactive feature demo relies on this differential compositing to keep high-frequency mouse wheel scrolling fluid and responsive.

---

## Formatted Printing & Markup

`CScreen::Print` supports embedded markup for foreground and background colours:

```cpp
// Printing with markup:
screen.Print(x, y, "[color=red]Danger![/color] [bg=blue]Mana restored.[/bg]");

// Clearing rectangular regions:
screen.ClearRect(RL::Rect(0, 0, 80, 25));
```

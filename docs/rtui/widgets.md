# Widgets: List & Input Windows {#rtui_widgets}

RTUI includes ready-to-use window widgets for common interactive patterns: selectable lists (`RL::CListWindow`) and text input prompts (`RL::CInputWindow`).

---

## Selectable List Window (`RL::CListWindow`)

The `rtui.list` module provides `RL::CListWindow`, a versatile selectable list widget that integrates with the window manager:

```cpp
import rtui.list;
import rtui.windows;

std::vector<std::string> items = {
    "Health Potion",
    "Iron Longsword",
    "Scroll of Teleportation",
    "Ration of Food"
};

auto list_win = std::make_shared<RL::CListWindow>(
    "Inventory",
    RL::Rect(10, 5, 35, 12),
    items
);
```

### Keyboard & Mouse Navigation
- **Navigation**: `Up` / `Down`, `Home` / `End`, `PageUp` / `PageDown`, and the mouse wheel move selection and automatically keep it scrolled into view.
- **Activation**: Pressing `Enter` or clicking an item activates it and triggers the callback configured via `setOnActivate([&](std::size_t index) { ... })`.
- **Dynamic Items**: `setItems(new_labels)` replaces the item list and resets selection. Programmatic selection is set with `Select(index)`.
- **Dimmed / Disabled Items**: `setEnabled(index, false)` dims an item. Users can select and view it, but clicking or pressing `Enter` will not activate it. `isEnabled(index)` queries state.
- **Markup Support**: Calling `setMarkup(true)` interprets colour markup inside label strings while preserving proper colours when scrolled horizontally.
- **Quick Shortcuts**:
  - `setShortcut(index, key)` assigns an individual key shortcut shown in bold in a dedicated column before the label. Pressing that key selects and activates the item.
  - `setLetterShortcuts()` automatically assigns shortcut letters `a`–`z`, then `A`–`Z` sequentially to all items.

---

## Text Input Window (`RL::CInputWindow`)

The `rtui.input` module provides `RL::CInputWindow`, a modal-style text input popup dialog:

```cpp
import rtui.input;

const std::size_t max_chars = 30;
auto input_win = std::make_shared<RL::CInputWindow>(
    "Enter Character Name",
    RL::Rect(20, 8, 40, 5),
    max_chars
);

input_win->setPrompt("[color=yellow]Name:[/color] ");
```

### Input Handling
- **Editing**: Typed characters are appended up to the specified character limit; `Backspace` deletes the preceding character.
- **Completion & Cancellation**:
  - Pressing `Enter` confirms entry. Query with `TakeEntered()` (returns `true` once).
  - Pressing `Escape` cancels entry. Query with `TakeCancelled()` (returns `true` once).
- **Text Retrieval**: Read or set the current UTF-8 string with `getText()` and `setText(str)`.

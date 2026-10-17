# Window Manager & Docking {#rtui_windows_docking}

The `rtui.windows` module provides a flexible window manager (`RL::CWindowManager`) supporting floating windows, edge docking, tabbed window groups, custom scrollbars, and event routing.

---

## Window Manager Basics

```cpp
import rtui.windows;

RL::CWindowManager windows;

// Create a window with title and initial floating rectangle
auto window = std::make_shared<RL::CWindow>("Inventory", RL::Rect(5, 5, 40, 20));
windows.AddWindow(window);
```

### Window Options (`RL::EWindowOptions`)
Window behavior can be restricted using bitwise flags:

| Flag | Meaning |
| :--- | :--- |
| `EWindowOptions::NoResize` | Disables border resizing drags and title double-click maximisation. |
| `EWindowOptions::NoMove` | Window position cannot be moved by dragging. |
| `EWindowOptions::NoClose` | Hides the close button and disables closing gestures. |
| `EWindowOptions::NoUndock` | Disallows dragging a docked tab out of its docking group. |

```cpp
auto win = std::make_shared<RL::CWindow>(
    "Map Legend",
    RL::Rect(0, 0, 30, 15),
    RL::EWindowOptions::NoResize | RL::EWindowOptions::NoClose
);
```

---

## Docking & Tab Groups

Windows can be docked against screen edges or grouped together into tabbed containers:
- **Tab Selection**: Clicking on a tab title focuses that window and activates its view.
- **Docking Policies**: 
  - `windows.setAllowTabUndocking(false)` prevents users from dragging a docked tab's title out of its group.
  - `windows.setAllowTabClosing(false)` hides close buttons on tab-grouped windows.
- **Resizing**: Full-width or docked windows remain resizable via boundary borders and corner resize grips. Docked windows can shrink across their dock edge while preserving the tab group.

---

## Event Routing & Focus

- **Focus Management**: The currently active window is given by `windows.getFocused()`. Clicking, opening, or raising a window focuses it. If a focused window closes, focus automatically falls back to the next topmost window. Programmatic focus is set via `windows.Focus(window)`.
- **Keyboard Handling**: Windows receive input by overriding `CWindow::HandleKey(const SKey&)`. From your main event loop, call `windows.HandleKey(key)`. If `HandleKey` returns `false`, the event was unhandled, allowing global application shortcuts.

---

## Proportional Scrollbars & Dragging

Windows can opt into scrollbar indicators and interactive mouse scrolling:

1. **Reporting Scroll State**: Override `getScrollInfo()` to return `SScrollInfo{content, viewport, offset}` in cell units. The manager automatically draws proportional indicators on the right and bottom borders when that axis overflows.
2. **Enabling Interactive Dragging**:
   - Override `supportsScrollDragging()` to return `true`.
   - Override `setScrollOffset(Position)` to apply the requested origin.
3. **Mouse Interaction**:
   - Dragging the thumb scrolls smoothly along the axis.
   - Clicking on the track pages towards that position.
   - Scrollbar tracks take precedence over border resizing, while corner resize grips remain accessible.

Roguelike Lib
=============

Roguelike Library is portable open-source library written in C++. It consist
of set of classes that can be used in all roguelike games. Classes are
categorized to fullfill tasks of random map generation, pathfinding,
counting field of view and making up names.

This project was was originally developed by Jakub Debski between 2006 and
2007. It was originally hosted on [SourceForge](https://sf.net/p/roguelikelib/)
and abandoned many years ago.

Licensed under the BSD License.

Terminal UI
-----------

Reusable terminal UI code lives in `rtui` (Roguelike Terminal UI), separate
from the core algorithms in `roguelikelib`. Include both directories in
Xmake and depend on the `RTUI` target, which publicly depends on
`RoguelikeLib`.

The UI modules are `rtui.screen`, `rtui.windows`, `rtui.mouse`,
`rtui.utf8`, `rtui.terminal`, `rtui.list`, and `rtui.map`; terminal colours are provided by `stc` in
`rtui/extern`. C++ types and functions retain their `RL` namespace.
Import `rtui.map` and call `RL::PrintMap(map)` to print a map (previously
`CMap::PrintMap()`). Import `rtui.utf8` explicitly when encoding glyphs;
`rl.tile` no longer re-exports it. The core library can be used without
terminal UI dependencies.

`CScreen::Compose(true_color, &previous_screen)` can emit only changed
cells compared with the last displayed frame. Identical frames emit nothing;
changed dimensions trigger a full redraw. The feature demo uses this to
keep wheel scrolling responsive and avoid output at viewport boundaries.

`rtui.map` also provides `DrawMap(screen, map, area, origin)`, which draws
map tiles into a screen rectangle from a map offset (default `(0, 0)`).
It respects the screen's clip and leaves cells outside the map untouched.
`CameraOrigin(map.getSize(), area.size, focus)` calculates an offset centred
on a focus position and clamped to map edges. `OnScreen(area, origin, pos)`
converts a map position to screen coordinates, returning `nullopt` outside
the viewport. These viewport helpers are adapted from Roguevalley.

`rtui.list` provides `CListWindow(title, floating_rect, labels)`, a selectable
list that can be docked and focused through `CWindowManager`. Up/Down,
Home/End and PageUp/PageDown move selection and keep it visible; the mouse
wheel moves selection too. Enter or a left click activates an item through
`setOnActivate(callback)`, passing its index. `getSelected()` returns an
optional index; empty lists have no selection. `Select(index)` allows
programmatic selection, and `setItems(labels)` replaces the list and resets
selection. Labels are literal UTF-8 text, clipped to the content width;
after `setMarkup(true)` they are text with colour markup, as `CScreen`
prints it, and keep their colours when scrolled sideways. `setEnabled(index,
false)` disables an item: it shows dimmed, and can be selected to look at
it, but Enter and clicks do not activate it; `isEnabled(index)` tells.
`setItems` enables all items again. `setOnSelect(callback)` follows the selection:
the callback is told the index of every item selected, by the keys, the
wheel, a click, dragging the scroll bar or `Select()`, and of the first
item of every new list - to show the selected item beside the list, say.

Windows can override `getScrollInfo()` to report `SScrollInfo{content,
viewport, offset}` in cells. The manager draws proportional position
indicators on the right and bottom frame borders only when that axis
overflows. The thumb is an inverse-video space, using ordinary terminal
attributes. To enable interaction, override `supportsScrollDragging()` to return true
and `setScrollOffset(Position)` to apply the requested viewport origin.
Dragging the thumb scrolls; clicking the track pages toward that position.
Scrollbar tracks take precedence over border resizing, while corner resize
grips remain available. Windows that do not opt in keep visual indicators.
The list widget and feature demo's map views support dragging both axes.
Dragging a list's vertical thumb keeps its selection within the visible
rows without activating it; horizontal dragging reveals clipped labels.

Window constructors accept `EWindowOptions`, combined with `|`, for
`NoResize`, `NoMove`, `NoClose`, and `NoUndock`. For example,
`CWindow(title, rect, EWindowOptions::NoResize | EWindowOptions::NoClose)`.
Existing constructor calls retain unrestricted behavior. `CListWindow`
accepts these options after its labels. Full-width windows remain
resizable: use the right border or top-right corner (outside scrollbar
tracks). Docked windows can shrink across their dock edge while retaining
their tab group; `NoResize` disables resize drags and title double-click
resizing. Explicit layout changes remain available.

Windows receive keyboard input by overriding `CWindow::HandleKey(const SKey&)`
and returning whether they consumed it. Call `windows.HandleKey(key)` from
the event loop; a false result lets the application handle global shortcuts.
The focused window is `windows.getFocused()`: opening, raising, or clicking
focuses a window and activates its tab; closing it falls back to the next
window on top. `windows.Focus(window)` explicitly focuses an open window.
Mouse, resize, closure, and timeout events are not routed as keyboard input.
Call `windows.setAllowTabUndocking(false)` to prevent dragging a docked
tab's title out of its group. This option defaults to true; the feature
demo disables it. Tab selection and explicit layout changes still work.
`windows.setAllowTabClosing(false)` hides close buttons for tab-group
windows and disables clicks on their former positions. It defaults to true;
the feature demo disables it. Explicit `Close`/`Toggle` calls still work.

`rtui.terminal` provides the POSIX terminal layer adapted from Roguevalley:
`RL::CTerminal` owns raw input mode, the alternate screen, cursor visibility,
mouse reporting, and restoration on destruction, exit, or termination signals.
Only one terminal object can be active at a time, and stdin and stdout must
both be terminals. `ReadKey(timeout_ms)` returns decoded Unicode keys, mouse
reports, resize events, or closure; `getSize()` returns columns and rows.
`DecodeKey()` can also decode buffered input without opening a terminal.

```cpp
import rtui.terminal;
import rtui.screen;

// Inside the application's main function:
RL::CTerminal terminal;
RL::CScreen screen(terminal.getSize());
screen.Print(0, 0, "Hello from RTUI");
terminal.Write(screen.Compose(true));
const auto key = terminal.ReadKey();
```

Credits
-------

Besides the original work of Jakub Debski, the library contains the work of
others, as listed in [AUTHORS](AUTHORS):

  * Kusigrosz wrote the delve map generator (`rl.mapgenerators.delve`), the
    winding path generator, and the name generator (`rl.names`), which makes
    up names that sound like those it is given, a list of English names, for
    instance. All of them were placed in the public
    domain; the original descriptions are kept at the top of each module.
  * Adam Milazzo wrote the field of view algorithm (`rl.fov`).
  * illyigan wrote the terminal colours code (stc).

Tests
-----

The tests of RoguelikeLib and RTUI are in `tests`, one binary with every
test, which [Xmake](https://xmake.io/) runs test by test:

```sh
cd tests
xmake test
```

`xmake test roguelikelib-tests/Screen` runs one test, and
`xmake test "roguelikelib-tests/*Tabs"` those whose names match. The tests
are those `tests/main.cpp` lists; `xmake.lua` finds their names there.
`xmake run roguelikelib-tests Screen Clock` runs some of them at once, and
tells of every failed check.

Demos
-----

There are two demos included with this library: a demo game and a feature
demo. Both can be built using [Xmake](https://xmake.io/):

```sh
cd demo-game
xmake config -m debug
xmake build
```

for the demo game and the following commands for the feature demo:

```sh
cd demo-features
xmake config
xmake build
```

You can run the generated executables with `xmake run`.

The feature demo is an interactive terminal application with six tabs:
Map generators, FOV, Path finding, Distance map, Scheduler, and Calendar.
Use `1`–`6`, Tab, or click a tab to switch. In Map generators, select a
generator with Up/Down, Home/End, PageUp/PageDown, or a click; Enter
regenerates it. The mouse wheel scrolls the map viewport. `R` regenerates any map demo, and `h/j/k/l`
pans the map. In FOV, arrows or a floor click move the observer. In Distance
map, click a floor to move the goal. Scheduler and Calendar advance automatically once per second while
visible. Enter/Space toggles pause/continue independently for each tab;
Calendar advances 30 minutes per tick, and `D` advances one day. Hidden
tabs and tabs obscured by the small-terminal message do not advance.
Resuming starts a fresh one-second interval without catching up. `Q` or Escape
quits. Resize the terminal to change the viewport. Maps remain at least 79
columns wide and expand to fill wider map viewports; a change in map width
regenerates the map. Map height remains 50 rows. All tab labels fit at
about 105 columns; keyboard selection works on smaller terminals too.

Pass a seed with `xmake run demo-features 123` for a reproducible sequence
of maps when using the same controls. The application needs an interactive
POSIX terminal of at least 40 columns and 10 rows.

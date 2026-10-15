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
`setShortcut(index, key)` gives an item a key of its own, shown in bold in a
column before the items, which stays when they scroll sideways; typed, it
selects the item and activates it, unless it is disabled. A key no item
has is not taken. `setLetterShortcuts()` gives the items, and every new
list of them, the letters a to z, then A to Z.

`rtui.input` provides `CInputWindow(title, floating_rect, longest)`, a
window to type a line of text into, after a prompt (`setPrompt`, as
markup): typed characters are added up to `longest`, Backspace takes the
last away, Enter enters the text and Escape cancels. `TakeEntered()` and
`TakeCancelled()` give what happened once; `setText` and `getText` set
and read what is typed, as UTF-8.

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
regenerates the map. Map height remains 50 rows by default. Use
`xmake run demo-features --size 200x100` (or
`xmake run demo-features 123 --size 200x100` with a seed) for a fixed custom
map size in all map tabs. Custom maps keep their dimensions across terminal
resizes; pan or scroll to explore them. The generator name shows the current
map dimensions beside it. Hover over a visible map tile to show its description
beside the generator name; moving outside the map clears the description.
Press **M** at runtime to open the map-size entry,
type dimensions such as `200x100`, and press Enter to apply them to all map
tabs. The popup shows the current dimensions; type the new ones and use
Backspace to edit. Escape cancels. Invalid dimensions leave the entry open.
The popup uses the shared `rtui.input` widget.
`--help` prints the command-line options. All tab labels fit at
about 105 columns; keyboard selection works on smaller terminals too.

Pass a seed with `xmake run demo-features 123` for a reproducible sequence
of maps when using the same controls. The application needs an interactive
POSIX terminal of at least 40 columns and 10 rows.

Town generation
---------------

`rl.mapgenerators.growthtown` (also exported by `rl.mapgenerators`) provides
`CreateGrowthTown(map, number_of_buildings = 80)`. It replaces the map with
an uneven town wall, a market square with a well, and winding main roads
three tiles wide. After buildings and gardens are placed, the generator finds the largest
open paved square in each quarter and puts a well at its centre. Main roads
and narrow lanes are excluded; quarters without a square at least 3×3 tiles
wide get no extra well. The market well remains, and all wells use the
`well` tile with the glyph `o`.
Cottages fill street frontages and back plots, separated
by shared one-tile passages. Remaining courtyards are paved; greenery within
the walls is limited to occasional two-tile gardens. The countryside stays
green. Rampart tiles overlap at steps to prevent movement through diagonal
gaps; the four gates have two-tile openings into the wider town roads. Building entrances face any of the
four directions. Most cottages measure 5–7 tiles along each axis, including
walls; smaller workshops and sheds fill narrow plots. The function returns
the buildings actually placed as `std::vector<SBuilding>`; requests above
the available space return fewer buildings. Wall corners are
inclusive, and `door` identifies each street-facing entrance. Empty and
small maps are supported; maps smaller than 19 cells on either axis
receive simple crossing paths without buildings. Use `InitRandomness(seed)`
for reproducible towns.

The interiors are inspired by [Lopes et al., A Constrained Growth Method
for Procedural Floor Plan Generation (GAMEON 2010)](https://graphics.tudelft.nl/~rafa/myPapers/bidarra.GAMEON10.pdf):
room seeds grow as rectangles, then absorb remaining space along their
frontiers according to relative area weights. Door placement favours a
larger public entrance room and connects the room adjacency graph. All
rooms are reachable from the street using cardinal steps when doors are
opened. This is a single-floor adaptation; it does not expose the paper's
hierarchical zones or user-defined adjacency constraints. Select **Growth
Town** in the feature demo to explore it.

Map stamping
------------

`destination.Stamp(source, Position(x, y))` copies a map into another map,
placing its top-left corner at `(x, y)`. It copies whole tiles, preserving
colours, glyphs, transparency and movement properties. The source stays
unchanged; overlapping self-stamps read a snapshot. Stamps are clipped at
the destination's right and bottom edges. Off-map origins and empty maps
are no-ops. Positions use the library's nonnegative coordinates.

An optional list of tile types leaves the corresponding destination cells
untouched. For example, keep wilderness under the grass outside a town:

```cpp
import rl.map;
import rl.position;
import rl.mapgenerators;
import std;

RL::CMap wilderness(RL::Size(500, 300));
RL::CreateVoronoiWilderness(wilderness);
RL::CMap town(RL::Size(79, 50));
RL::CreateGrowthTown(town);
const std::array<std::string_view, 1> skip{"grass"};
wilderness.Stamp(town, RL::Position(210, 120), skip);
```

Omit `skip` to copy the entire rectangular map, including its grass.
Stamping copies terrain; callers translate any returned building positions
by the stamp offset when keeping that metadata. The feature demo includes
**Town in Wilderness**, showing a Growth Town stamped over a forest.

To connect a Growth Town to an existing wilderness road network, import
`rl.maputils` and use the reported local gate positions:

```cpp
std::vector<RL::Position> gates;
RL::CreateGrowthTown(town, 80, &gates);
const auto connected = RL::StampTownWithRoads(wilderness, town,
                                            RL::Position(210, 120), gates);
```

Town roads stop at the ramparts. `StampTownWithRoads` skips grass, preserves
the stamped town, and routes single-tile connections from its gates to
surviving wilderness `corridor` or `bridge` tiles. It prefers clear ground,
can clear trees and rocks, and adds bridges over water when needed. It
returns the number of gates connected; gates with no reachable road are
left without external roads. The Town in Wilderness demo uses this helper.

Forest roads enter clearings near their edges, leaving the centres open.
Routes heading in similar directions share entrances and existing road
segments before branching; nearby roads merge instead of running in parallel.
Unpaved access inside each clearing stays walkable, and river crossings use
bridges.

Voronoi wilderness forests use a lighter mix comparable to the forest
generator: scattered trees, passable underbrush (`plant`), open grass and
occasional rocks. Density varies gradually across each forest, creating
thickets and thinner patches rather than continuous blocks of trees.

Walkable Voronoi hills use the plains' grass glyph, shaded continuously
from green low ground to muted ochre at higher elevations. They contain
patches of passable `scrub`, occasional flowering `heather`, and about 2%
scattered rocks. The ground keeps its `slope` tile type for terrain rules.

Some larger hills have short, faint animal trails (`animal_trail`, `.`),
with long gentle stretches and occasional bends. Only about 30% of eligible
hills get a trail, limited to 6–16 tiles; most hills remain undisturbed. Occasional fallen logs span
2–3 tiles, shown as `=` horizontally or `|` vertically. Trails and logs
are passable; logs do not overwrite trails, rocks, buildings or other biomes.
Both features follow the map seed.

The Voronoi demo uses automatic region counts: approximately one region per
200 tiles, with at least 10. Larger maps add regions to keep terrain patches
at a similar scale. Explicit nonzero region counts still select a fixed
count. Voronoi town buildings are 4–7 tiles wide and 4–7 tiles high, including
walls; larger town regions hold multiple buildings, with one tile of open
ground between houses and their neighbours. All buildings are returned in
`SWildernessInfo::buildings`. The feature demo tosses a coin on each
Voronoi generation, including `R` regeneration, to enable or disable
buildings. Its status line reports the selected mode. Without buildings,
field regions become walkable bogs with reed beds and shallow pools;
hedges, farm gates and their agricultural road connections are omitted.

Voronoi chasms are branching, curved ravines with open grassy shoulders.
Broken rocky rims (`#` and `^`) fade through descending faces (`:`) into
subdued depths (`·`). Depth shading follows the actual ravine edges.
`chasm_rim`, `chasm_wall` and `chasm` are all impassable and are never
opened by wilderness connectivity repairs.

Voronoi town doors face the closest neighbouring building. Their outside
approaches are joined by a shared network of narrow paths, routed around
all building footprints. Paths stay within the town where possible and
can follow repaired walkable terrain around fragmented town regions.

Field entrances face the village and connect to its shared road network
using `BuildWindingPath`, with safe routing around buildings, hedges and
impassable terrain. Roads merge with earlier connections and bridge water
where necessary. Crossings leave cultivated field tiles unchanged and
continue beyond the field. Gate positions are returned in
`SWildernessInfo::field_gates`.

Road variants in `rl.maputils`: `BuildZigzagPath`/`AddZigzagRoad` produce
angular random routes; `BuildSigsagPath`/`AddSigsagRoad` cut their one-cell
corners but retain large zigzag bends. `BuildWindingPath`/`AddWindingRoad`
perturb spaced waypoints with turn-angle limits, producing gentler winding
roads even between endpoints on the same row or column. The build helpers
return tiles for terrain-aware placement; the add helpers overwrite terrain.

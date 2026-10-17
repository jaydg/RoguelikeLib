# RoguelikeLib & RTUI

[![Deploy Documentation](https://github.com/RoguelikeLib/RoguelikeLib/actions/workflows/docs.yml/badge.svg)](https://github.com/RoguelikeLib/RoguelikeLib/actions/workflows/docs.yml)
[![License](https://img.shields.io/badge/license-BSD-blue.svg)](LICENSE)

A modern C++20 toolkit for roguelike games and terminal applications.

This repository provides **two independent libraries under one roof**:

1. **RoguelikeLib Core (`roguelikelib`)**  
   A standalone, portable C++20 algorithmic library providing procedural
   map generation, field of view (FOV), pathfinding, distance maps,
   scheduling, name generation, and high-performance PRNG.  
   **RoguelikeLib has zero UI or terminal dependencies** and can be
   integrated into any game engine, console tool, or graphical
   framework.

2. **Roguelike Terminal UI (`rtui`)**  
   An independent C++20 terminal UI framework for POSIX environments.
   It provides raw terminal mode ownership, differential screen buffer
   compositing, a docking window manager with tab groups, mouse-drag
   scrollbars, interactive widgets (lists, text input), and map
   viewport rendering.

Originally developed by Jakub Debski (2006–2007) and hosted on
SourceForge, the library has been revitalized with modern C++20 modules,
comprehensive test suites, and extensive procedural generation
capabilities.

---

## Documentation

Full documentation, user guides, and API reference are generated with
Doxygen and published to GitHub Pages:

📖 **[Online Documentation](https://joachim.github.io/RoguelikeLib/)**  
*(or open `build/docs/html/index.html` after running `doxygen`)*

The documentation is split into separate tracks for each library:

### RoguelikeLib Core Guides
- **[Getting Started](docs/roguelikelib/getting_started.md)**:
  Library architecture, Xmake targets, and importing C++20 modules.
- **[Core Algorithms & Randomness](docs/roguelikelib/algorithms.md)**:
  Field of view, A* pathfinding, distance maps, and Xoshiro256++ PRNG.
- **[Town Generation](docs/roguelikelib/town_generation.md)**:
  Fortified towns, ramparts, gates, market squares, and constrained room
  growth (GAMEON 2010).
- **[Map Stamping & Wilderness](docs/roguelikelib/map_stamping.md)**:
  Map stamping, automated road networks, Voronoi biomes, animal trails,
  and chasms.

### RTUI Guides
- **[Getting Started](docs/rtui/getting_started.md)**:
  UI architecture and writing your first interactive terminal program.
- **[Screen & Terminal Lifecycle](docs/rtui/screen_terminal.md)**:
  POSIX terminal ownership (`RL::CTerminal`), signal safety, and
  differential frame updates (`RL::CScreen`).
- **[Window Manager & Docking](docs/rtui/windows_docking.md)**:
  Docking, tabbed containers, window options, focus routing, and
  scrollbar dragging.
- **[Widgets: List & Input](docs/rtui/widgets.md)**:
  Selectable lists (`RL::CListWindow`) and text input prompts
  (`RL::CInputWindow`).
- **[Map Viewport & Camera](docs/rtui/map_viewport.md)**:
  Map viewport blitting (`DrawMap`), camera tracking (`CameraOrigin`),
  and screen coordinates (`OnScreen`).

---

## Integration via Xmake

RoguelikeLib and RTUI use [Xmake](https://xmake.io/) as their build
system. You can include either library in your `xmake.lua`:

### Using RoguelikeLib Alone (Zero UI Dependencies)

```lua
includes("path/to/RoguelikeLib/roguelikelib")

target("MyGame")
    set_kind("binary")
    set_languages("c++20")
    add_deps("RoguelikeLib")
    add_files("src/**.cpp")
```

```cpp
import rl.map;
import rl.fov;
import rl.mapgenerators.growthtown;
import rl.randomness;

RL::CMap map(RL::Size(80, 50));
RL::CreateGrowthTown(map);
```

### Using RTUI (Terminal UI)

```lua
includes("path/to/RoguelikeLib/roguelikelib")
includes("path/to/RoguelikeLib/rtui")

target("MyTerminalApp")
    set_kind("binary")
    set_languages("c++20")
    add_deps("RTUI") -- Publicly depends on RoguelikeLib
    add_files("src/**.cpp")
```

```cpp
import rtui.terminal;
import rtui.screen;

RL::CTerminal terminal;
RL::CScreen screen(terminal.getSize());
screen.Print(2, 2, "Hello from RTUI");
terminal.Write(screen.Compose(true));
auto key = terminal.ReadKey();
```

---

## Running Tests

The test suite covers both RoguelikeLib and RTUI:

```sh
cd tests
xmake test
```

To run a specific test suite or test matching a pattern:

```sh
xmake test roguelikelib-tests/Screen
xmake test "roguelikelib-tests/*Tabs"
```

---

## Interactive Demos

Two interactive demos are included:

### Feature Demo (`demo-features`)
An interactive terminal application showcasing six core features across
selectable tabs:
1. **Map generators**: Interactive generation and exploration of Delve,
   Caves, Maze, Mines, Simple City, Space Shuttle, Standard Dungeon,
   Growth Town, Voronoi Wilderness, and Town in Wilderness.
2. **FOV**: Interactive field of view calculation with moving observers.
3. **Path finding**: Real-time A* path calculation.
4. **Distance map**: Dynamic Dijkstra distance field visualizer.
5. **Scheduler**: Turn-based action scheduler.
6. **Calendar**: In-game calendar and clock simulator.

```sh
cd demo-features
xmake config
xmake build
xmake run demo-features
```

*Controls*: Use `1`–`6` or `Tab` to switch tabs; `R` to regenerate maps;
`h`/`j`/`k`/`l` or arrow keys to pan; mouse wheel to scroll; `M` to set
custom map dimensions (e.g. `200x100`); `Q` or `Esc` to quit. Run with
`--help` for command-line options.

### Demo Game (`demo-game`)

```sh
cd demo-game
xmake config -m debug
xmake build
xmake run demo-game
```

---

## Generating Documentation Locally

To generate the documentation locally:

```sh
doxygen Doxyfile
```

The resulting HTML site will be generated at
`build/docs/html/index.html`. Open it in any web browser to view the
interactive documentation.

---

## Credits & License

Besides the original work of Jakub Debski, the library incorporates work
from several contributors (see [AUTHORS](AUTHORS)):
- **Kusigrosz**: Delve map generator (`rl.mapgenerators.delve`),
  winding path generator, and Markov name generator (`rl.names`).
- **Adam Milazzo**: Field of view algorithm (`rl.fov`).
- **illyigan**: Terminal color formatting (`stc`).
- **Sebastiano Vigna & David Blackman**: PRNG engine based on
  xoshiro256++ 1.0 (`rl.extern.xoshiro256`).

Licensed under the [BSD 3-Clause License](LICENSE).

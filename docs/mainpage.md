# RoguelikeLib & RTUI Documentation {#mainpage}

Welcome to the documentation for **RoguelikeLib** and **RTUI**.

> [!IMPORTANT]
> **Two Independent Libraries Under One Roof**
> 
> This repository provides two decoupled C++20 libraries:
> 1. **RoguelikeLib Core (`roguelikelib`)**: An algorithmic roguelike toolkit covering procedural map generation, field-of-view, pathfinding, distance transforms, scheduling, randomness, and name generation. **It has zero terminal or graphical UI dependencies** and can be integrated into any game engine, console tool, or graphical framework.
> 2. **Roguelike Terminal UI (`rtui`)**: A standalone terminal UI and windowing library providing raw POSIX terminal abstraction, differential screen compositing, a docking window manager with tab groups, scrollbar dragging, interactive widgets, and map viewport rendering.

---

## Documentation Sections

The documentation is organized into two distinct guide tracks followed by the unified API reference.

### RoguelikeLib Core Guides

Algorithms and procedural generation tools with zero graphical dependencies:

- **\ref roguelikelib_getting_started "Getting Started with RoguelikeLib"**: Library architecture, Xmake target integration, and importing C++20 modules.
- **\ref roguelikelib_town_generation "Procedural Town Generation"**: GrowthTown generator, town walls, gates, market squares, and procedural room growth based on Lopes et al. (GAMEON 2010).
- **\ref roguelikelib_map_stamping "Map Stamping & Wilderness"**: Map stamping API, automated road networks, Voronoi wilderness biomes, hill trails, and chasms.
- **\ref roguelikelib_algorithms "Core Algorithms & Randomness"**: Field-of-view, pathfinding, distance maps, and high-performance PRNG with Xoshiro256++.

### RTUI (Terminal UI) Guides

Terminal abstraction and rich text-mode interface components:

- **\ref rtui_getting_started "Getting Started with RTUI"**: UI architecture, Xmake target dependency, and building your first terminal application.
- **\ref rtui_screen_terminal "Terminal Lifecycle & Screen Compositing"**: POSIX terminal ownership (`RL::CTerminal`), signal safety, and differential frame compositing (`RL::CScreen`).
- **\ref rtui_windows_docking "Window Manager & Docking"**: Floating and docked windows, tab groups, undocking rules, focus handling, and scrollbar dragging.
- **\ref rtui_widgets "Widgets: List & Input Windows"**: Ready-to-use widgets: selectable lists (`RL::CListWindow`) and text input prompts (`RL::CInputWindow`).
- **\ref rtui_map_viewport "Map Viewport & Camera"**: Rendering maps into screen viewports (`DrawMap`), camera focus tracking (`CameraOrigin`), and coordinate conversion (`OnScreen`).

---

## API Reference

Browse the extracted API documentation generated directly from the C++20 module code:

- [**C++20 Modules**](modules.html): Index of all exported modules (`rl.*` and `rtui.*`).
- [**Class & Struct Index**](annotated.html): Alphabetical list of classes, structs, and interfaces.
- [**Namespaces**](namespaces.html): All types organized under the `RL` namespace.

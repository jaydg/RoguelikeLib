# Getting Started with RoguelikeLib {#roguelikelib_getting_started}

**RoguelikeLib** is a lightweight, portable C++20 library providing core algorithms and data structures for roguelike games and simulations.

> [!NOTE]
> RoguelikeLib contains **zero UI or terminal dependencies**. It operates purely on data structures such as `RL::CMap`, `RL::Position`, `RL::Size`, and algorithms for map generation, pathfinding, field of view, scheduling, and randomness.

---

## Target Integration with Xmake

RoguelikeLib uses [Xmake](https://xmake.io/) as its build system. To use RoguelikeLib in your project, include the `roguelikelib` directory and add `RoguelikeLib` as a dependency:

```lua
includes("path/to/RoguelikeLib/roguelikelib")

target("MyGame")
    set_kind("binary")
    set_languages("c++20")
    add_deps("RoguelikeLib")
    add_files("src/**.cpp")
```

---

## C++20 Module Structure

All RoguelikeLib components are exported as C++20 modules under the `rl.*` namespace hierarchy:

| Module | Purpose | Key Symbols |
| :--- | :--- | :--- |
| `rl.position` | 2D Coordinates, sizes, directions, and areas | `RL::Position`, `RL::Size`, `RL::Rect`, `RL::Direction` |
| `rl.tile` | Map cell representations, flags, and glyphs | `RL::CTile`, `RL::ETileFlags` |
| `rl.map` | 2D grid map container and stamping | `RL::CMap` |
| `rl.matrix` | Generic 2D matrix container | `RL::CMatrix<T>` |
| `rl.randomness` | High-performance RNG (Xoshiro256++) | `RL::Random()`, `RL::InitRandomness()`, `RL::RandomInt()` |
| `rl.fov` | Field of View computation | `RL::CalculateFov()` |
| `rl.pathfinding` | A* and Dijkstra pathfinding | `RL::FindPath()`, `RL::BuildDistanceMap()` |
| `rl.mapgenerators` | Procedural map generators umbrella module | `rl.mapgenerators.*` |
| `rl.maputils` | Road construction, carving, and stitching | `RL::BuildWindingPath()`, `RL::StampTownWithRoads()` |
| `rl.scheduler` | Turn-based scheduling and event queues | `RL::CScheduler` |
| `rl.names` | Markov-chain procedural name generator | `RL::CNameGenerator` |

---

## Quick Example: Generating a Map

Here is a minimal example using RoguelikeLib to create a map, populate it with a procedural delve dungeon, and compute field of view:

```cpp
import rl.map;
import rl.position;
import rl.mapgenerators.delve;
import rl.fov;
import rl.randomness;
import std;

int main() {
    // 1. Seed the RNG
    RL::InitRandomness(12345);

    // 2. Create an 80x40 map
    RL::CMap map(RL::Size(80, 40));

    // 3. Generate a dungeon
    RL::CreateDelveDungeon(map);

    // 4. Compute field of view from a player position
    RL::Position player(40, 20);
    const int radius = 10;
    RL::CalculateFov(map, player, radius);

    // 5. Inspect visible cells
    for (int y = 0; y < map.getSize().h; ++y) {
        for (int x = 0; x < map.getSize().w; ++x) {
            if (map.GetTile(RL::Position(x, y)).isVisible()) {
                // Render or inspect cell...
            }
        }
    }

    return 0;
}
```

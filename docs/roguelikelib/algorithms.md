# Core Algorithms & Randomness {#roguelikelib_algorithms}

RoguelikeLib provides high-performance, thoroughly-tested implementations of core roguelike algorithms.

---

## Field of View (`rl.fov`)

The field of view module provides precise, symmetric line-of-sight calculations adapted from Adam Milazzo's restrictive quadrant algorithm.

```cpp
import rl.map;
import rl.position;
import rl.fov;

RL::CMap map(RL::Size(80, 50));
RL::Position observer(25, 15);
const int view_radius = 12;

// Updates tile visibility flags on map
RL::CalculateFov(map, observer, view_radius);

// Check if a tile was revealed
if (map.GetTile(RL::Position(28, 18)).isVisible()) {
    // Tile is within line of sight
}
```

---

## Pathfinding & Distance Transforms (`rl.pathfinding`)

The pathfinding module provides optimal and heuristic routing:

### 1. A* Pathfinding (`RL::FindPath`)
Computes the shortest passable path between two positions:

```cpp
import rl.map;
import rl.position;
import rl.pathfinding;

RL::Position start(10, 10);
RL::Position goal(45, 30);

std::vector<RL::Position> path = RL::FindPath(map, start, goal);
if (!path.empty()) {
    // Step-by-step coordinates from start to goal
}
```

### 2. Distance Transforms (`RL::BuildDistanceMap`)
Computes a Dijkstra flow field / distance map from one or more target goals across the map:

```cpp
RL::CMatrix<int> dist_map = RL::BuildDistanceMap(map, goal);

// Get distance to goal from any tile:
int distance_in_steps = dist_map.Get(RL::Position(15, 20));
```

---

## High-Performance Randomness (`rl.randomness`)

RoguelikeLib includes a fast, 64-bit pseudo-random number generator based on Sebastiano Vigna and David Blackman's **xoshiro256++ 1.0**:

- **PRNG Engine**: Uses `Xoshiro256PlusPlus` seeded through a `SplitMix64` state mixer, replacing slower legacy generators.
- **Period**: $2^{256} - 1$ states, passing BigCrush and standard randomness test suites.
- **Reproducibility**: Explicitly seedable via `RL::InitRandomness(seed)`.

```cpp
import rl.randomness;

// Initialize generator with a seed
RL::InitRandomness(1337);

// Draw random numbers:
double real_val = RL::Random();               // [0.0, 1.0)
int dice_roll   = RL::RandomInt(1, 6);         // [1, 6] inclusive
std::size_t idx = RL::RandomIndex(my_vec.size()); // [0, size - 1]
```

---

## Procedural Name Generator (`rl.names`)

A Markov-chain name generator adapted from Kusigrosz:

```cpp
import rl.names;

RL::CNameGenerator names;
names.Learn("Arthur", "Lancelot", "Gawain", "Percival", "Galahad");

std::string hero_name = names.GenerateName();
```

---

## In-Game Calendar & Date System (`rl.calendar`)

The calendar module provides `RL::CCalendar` for custom in-game year structures, translating sequential elapsed day counters into structured calendar dates:

- **Custom Year Structure**: Build a year from any number of parts (`RL::SCalendarPart`) with arbitrary names and durations (months, seasons, moon cycles, festivals).
- **Epoch / Day-of-Year Offset**: Configure which day of the year the timeline begins counting from.
- **Structured Dates**: `RL::CCalendar::DateOf()` returns an `RL::SDate` containing 1-based year, 0-based part index, and 1-based day of month.
- **Ordinal Formatting**: `RL::Ordinal()` generates English ordinal strings (e.g., `"1st"`, `"2nd"`, `"3rd"`, `"11th"`, `"22nd"`).

```cpp
import rl.calendar;
import std;

// 1. Define a 4-season calendar starting on day 10 of the year
RL::CCalendar calendar({
    {"Thawmoon", 30},
    {"Seedmoon", 30},
    {"Sunmoon", 30},
    {"Frostmoon", 30}
}, 10);

// 2. Query date from elapsed days (e.g. from RL::CClock::Day())
RL::SDate date = calendar.DateOf(35); // 35 days after start
// date.year == 1, date.part == 1 ("Seedmoon"), date.day == 16

// 3. Format as a display string with ordinal suffix
std::string formatted = std::format("Year {}, {} of {}",
    date.year, RL::Ordinal(date.day), calendar.PartName(date.part));
// "Year 1, 16th of Seedmoon"
```

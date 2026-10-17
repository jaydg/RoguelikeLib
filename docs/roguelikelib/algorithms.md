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

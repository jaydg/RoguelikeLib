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

---

## Turn-Based Clock & Daily Schedules (`rl.clock`)

The clock module provides `RL::CClock` for tracking time of day in turn-based worlds and `RL::CDailySchedule` for recurring daily phases and character routines:

- **Turn-to-Time Progression**: Advance by a configurable number of minutes per turn (`RL::CClock::Advance()`).
- **Time Queries**: Query hour (0-23), minute (0-59), minute of the day (0-1439), and elapsed days (0 on day 1).
- **Serialization**: Simple state saving and restoring via `RL::CClock::Minutes()` and `RL::CClock::FromMinutes()`.
- **Timestamp Formatting**: `RL::CClock::ToString()` produces standard `"HH:MM"` 24-hour time strings.
- **Daily Schedules**: `RL::CDailySchedule<T>` maps times of day to states (e.g. dawn, day, dusk, night) with automatic wrap-around past midnight.

```cpp
import rl.clock;
import std;

// 1. Create a clock with 1 minute per turn, starting at 08:00
RL::CClock clock(1, 8, 0);

// 2. Set up recurring daily phases
RL::CDailySchedule<std::string_view> day;
day.Set(6, 0, "morning");
day.Set(12, 0, "afternoon");
day.Set(18, 0, "evening");
day.Set(22, 0, "night");

// 3. Advance turns (e.g. 5 hours = 300 turns)
clock.Advance(300);

clock.ToString();   // "13:00"
clock.Hour();       // 13
clock.Day();        // 0 (first day)
day.At(clock);      // "afternoon"
```

---

## Discrete Timeline Scheduler (`rl.scheduler`)

The scheduler module provides `RL::CScheduler<Id>` to drive turn order in turn-based games using speed or action-cost systems:

- **Speed / Energy Systems**: Faster actors can act with shorter tick delays (`cost * 100 / speed`), taking turns more frequently.
- **Deterministic Turn Order**: Actors scheduled for the exact same tick act strictly in the order they were scheduled (FIFO tie-breaking).
- **Identifier Decoupling**: Stores copyable actor handles (`Id`) such as pointers, integer entity IDs, or strings without taking ownership.
- **Dynamic Updates & Removal**: Re-scheduling an actor adjusts their scheduled time; `Remove()` cancels turns in @f$O(\log N)@f$.
- **Complete Serialization**: Inspect upcoming turns with `Scheduled()` and restore timeline state with `Restore()`.

```cpp
import rl.scheduler;

struct Actor {
    int speed = 100; // base speed
};

Actor player{.speed = 100};
Actor fast_goblin{.speed = 200};

RL::CScheduler<Actor*> timeline;
timeline.Schedule(&player, 0);
timeline.Schedule(&fast_goblin, 0);

// Main turn loop:
while (auto actor = timeline.Next()) {
    // Current simulation tick: timeline.Now()
    const RL::Ticks base_cost = 100;
    const RL::Ticks delay = base_cost * 100 / (*actor)->speed;

    // Reschedule actor for their next turn:
    timeline.Schedule(*actor, delay);
    break;
}
```

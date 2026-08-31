# Theta* Pathfinding Module

This module implements the Theta* algorithm for any-angle path planning on grids, as described in the paper "Theta*: Any-Angle Path Planning on Grids" by Daniel et al. (arXiv:1401.3843).

## Overview

Theta* extends the A* algorithm to find paths that are not constrained to grid edges, allowing for shorter, more natural paths in grid-based environments. This is particularly useful for:

- Games with grid-based movement where you want more natural-looking paths
- Robotics applications on discrete grids
- Any application where path optimality is important

## Features

- **Basic Theta***: Simple and fast implementation that finds short paths
- **Angle-Propagation Theta***: More complex variant with better worst-case complexity
- **Line of Sight Checking**: Uses Bresenham's algorithm to check visibility between grid cells
- **Customizable**: Support for custom heuristic and line-of-sight functions
- **Grid-based**: Works seamlessly with RoguelikeLib's map system

## Usage

### Basic Usage

```cpp
#include <vector>
import rl.map;
import rl.position;
import rl.theta_star;

using namespace RL;

// Create a map
CMap game_map(Size(20, 20));
game_map.Clear("room");

// Add some obstacles
game_map.SetCell(5, 5, "wall");
game_map.SetCell(10, 10, "wall");

// Define start and end positions
Position start(1, 1);
Position end(18, 18);

// Find path using Basic Theta*
std::vector<Position> path;
CThetaStar theta_star;
bool path_found = theta_star.FindPath(game_map, start, end, path, true);

if (path_found) {
    // Path contains the sequence of positions from start to end
    for (const auto& pos : path) {
        std::cout << pos.toString() << std::endl;
    }
}
```

### Angle-Propagation Theta*

```cpp
// Use Angle-Propagation variant
std::vector<Position> path_ap;
bool path_found_ap = theta_star.FindPathAP(game_map, start, end, path_ap, true);
```

### Convenience Functions

```cpp
// One-liner for Basic Theta*
std::vector<Position> path;
bool found = ThetaStarFindPath(game_map, start, end, path, true);

// One-liner for Angle-Propagation Theta*
std::vector<Position> path_ap;
bool found_ap = ThetaStarFindPathAP(game_map, start, end, path_ap, true);
```

### Line of Sight Checking

```cpp
// Check if two positions have line of sight
Position pos1(1, 1);
Position pos2(10, 10);
SLineOfSightResult los = CheckLineOfSight(game_map, pos1, pos2);

if (los.visible) {
    std::cout << "Positions have line of sight" << std::endl;
} else {
    std::cout << "Line of sight blocked at: " << los.blocking_pos.toString() << std::endl;
}
```

### Custom Heuristic Function

```cpp
// Set a custom heuristic (e.g., Manhattan distance)
theta_star.SetHeuristicFunction([](const Position& a, const Position& b) {
    double dx = std::abs(static_cast<int>(a.x) - static_cast<int>(b.x));
    double dy = std::abs(static_cast<int>(a.y) - static_cast<int>(b.y));
    return dx + dy; // Manhattan distance
});
```

### Custom Line of Sight Function

```cpp
// Set a custom line of sight function
theta_star.SetLineOfSightFunction(
    [](const CMap& level, const Position& start, const Position& end) {
        // Your custom LOS implementation
        // Return SLineOfSightResult(true) if visible, 
        // SLineOfSightResult(false, blocking_pos) if blocked
    }
);
```

## API Reference

### Classes

#### `CThetaStar`

Main Theta* pathfinding class.

**Methods:**

- `FindPath(const CMap&, const Position&, const Position&, std::vector<Position>&, bool)`
  - Basic Theta* path finding
  - Parameters: map, start, end, output path, allow diagonals
  - Returns: true if path found

- `FindPathAP(const CMap&, const Position&, const Position&, std::vector<Position>&, bool)`
  - Angle-Propagation Theta* path finding
  - Parameters: same as FindPath

- `SetLineOfSightFunction(std::function<...>)`
  - Set custom line of sight function

- `SetHeuristicFunction(std::function<...>)`
  - Set custom heuristic function

#### `SLineOfSightResult`

Structure containing line of sight check results.

**Members:**
- `bool visible`: Whether there is line of sight
- `Position blocking_pos`: Position of blocking obstacle (if any)

### Functions

- `ThetaStarFindPath(const CMap&, const Position&, const Position&, std::vector<Position>&, bool)`
  - Convenience function for Basic Theta*

- `ThetaStarFindPathAP(const CMap&, const Position&, const Position&, std::vector<Position>&, bool)`
  - Convenience function for Angle-Propagation Theta*

- `CheckLineOfSight(const CMap&, const Position&, const Position&)`
  - Standalone line of sight check function

## Algorithm Details

### Basic Theta*

Basic Theta* works similarly to A* but with a key difference: when expanding a node, it checks if there's line of sight between the current node's parent and the neighbor. If there is, it considers creating a direct path from the parent to the neighbor, potentially bypassing the current node entirely.

This allows the algorithm to find paths that cut across open areas rather than being constrained to grid edges.

### Angle-Propagation Theta*

Angle-Propagation Theta* extends Basic Theta* by propagating angle ranges along with path information. This helps ensure that the algorithm finds true shortest paths in more cases, at the cost of increased complexity and runtime.

## Performance

- **Basic Theta***: Fast, finds short paths, but not guaranteed to find true shortest paths
- **Angle-Propagation Theta***: Better worst-case complexity, finds slightly longer paths, more complex

Both variants have runtime comparable to A* on grids, with Theta* typically finding shorter paths.

## Comparison with A*

| Algorithm | Path Quality | Speed | Memory | Any-Angle |
|-----------|--------------|-------|--------|------------|
| A* | Good | Fast | Low | No |
| Basic Theta* | Better | Fast | Medium | Yes |
| AP Theta* | Best | Medium | High | Yes |

## Limitations

- The current implementation uses Euclidean distance for edge costs
- Line of sight checking uses Bresenham's algorithm, which may not be optimal for all use cases
- Angle-Propagation implementation is simplified compared to the full algorithm described in the paper

## References

- Daniel, K., Nash, A., Koenig, S., & Felner, A. (2010). Theta*: Any-Angle Path Planning on Grids. Journal of Artificial Intelligence Research, 39, 533-579.
- arXiv:1401.3843 [cs.CG]

## License

This module is part of RoguelikeLib and is licensed under the same terms as the main library.
//////////////////////////////////////////////////////////////////////////
// Path finding
//////////////////////////////////////////////////////////////////////////
module;

export module rl.pathfinding;

import rl.map;
import rl.maputils;
import rl.matrix;
import rl.position;
import std;

export namespace RL
{

// Finds a path from start to end on the level.
// Returns true and writes the sequential steps (excluding start) to path if reachable.
bool FindPath(
    const CMap &level,
    const Position& start,
    const Position& end,
    std::vector<Position>& path,
    bool diagonals = true)
{
    auto pathmap = CMatrix<int>(level.getSize(), std::numeric_limits<int>::max());

    for (std::size_t x = 0; x < level.getWidth(); x++) {
        for (std::size_t y = 0; y < level.getHeight(); y++) {
            if (level.get_unchecked(x, y).isPassable()) {
                pathmap.set_unchecked(x, y, std::numeric_limits<int>::max() - 1);
            }
        }
    }

    // fill from end to start, with the same moves the walk back may take
    if (!pathmap.FloodFill(end, 0, diagonals, 1, start)) {
        return false;
    }

    // Walk from start to end following the gradient
    static constexpr std::array<std::pair<int, int>, 4> cardinalSteps = {{
        {0, -1}, {-1, 0}, {1, 0}, {0, 1}
    }};
    static constexpr std::array<std::pair<int, int>, 4> diagonalSteps = {{
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    }};

    Position current = start;

    while (current != end) {
        int best_value = pathmap.get_unchecked(current.x, current.y);
        Position best_pos = current;

        auto check_neighbor = [&](int dx, int dy) {
            int nx = static_cast<int>(current.x) + dx;
            int ny = static_cast<int>(current.y) + dy;
            if (nx >= 0 && ny >= 0 && nx < static_cast<int>(pathmap.getWidth()) && ny < static_cast<int>(pathmap.getHeight())) {
                int val = pathmap.get_unchecked(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
                if (val < best_value) {
                    best_value = val;
                    best_pos = Position(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
                }
            }
        };

        // Straight steps first for natural path alignment
        for (const auto& [dx, dy] : cardinalSteps) {
            check_neighbor(dx, dy);
        }
        if (diagonals) {
            for (const auto& [dx, dy] : diagonalSteps) {
                check_neighbor(dx, dy);
            }
        }

        if (best_pos == current) {
            path.clear();
            return false;
        }

        current = best_pos;
        path.push_back(current);
    }

    return true;
}

} // namespace RL

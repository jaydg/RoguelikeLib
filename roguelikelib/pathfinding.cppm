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

// Internal search node for A* priority queue
struct SPathNode {
    Position pos;
    int g;
    int f;

    bool operator>(const SPathNode& other) const noexcept
    {
        if (f != other.f) {
            return f > other.f;
        }
        return g < other.g; // Tie-breaker: prefer nodes closer to target
    }
};

struct SCellState {
    std::uint32_t search_id = 0;
    int g = std::numeric_limits<int>::max();
    Position parent;
};

// Finds a path from start to end on the level.
// Returns true and writes the sequential steps (excluding start) to path if reachable.
bool FindPath(
    const CMap &level,
    const Position& start,
    const Position& end,
    std::vector<Position>& path,
    bool diagonals = true)
{
    path.clear();

    const std::size_t width = level.getWidth();
    const std::size_t height = level.getHeight();

    if (width == 0 || height == 0 || !level.inside(start) || !level.inside(end)) {
        return false;
    }

    if (start == end) {
        return true;
    }

    // Reusable generational scratchpad to avoid reallocating and clearing on every search
    thread_local std::vector<SCellState> grid;
    thread_local std::uint32_t current_search_id = 0;

    const std::size_t total_cells = width * height;
    if (grid.size() != total_cells) {
        grid.assign(total_cells, SCellState{});
        current_search_id = 0;
    }

    ++current_search_id;
    if (current_search_id == 0) {
        // Reset on 32-bit counter wrap-around
        grid.assign(total_cells, SCellState{});
        current_search_id = 1;
    }

    const bool target_passable = level.get_unchecked(end.x, end.y).isPassable();
    const int target_x = static_cast<int>(end.x);
    const int target_y = static_cast<int>(end.y);

    auto heuristic = [&](int x, int y) noexcept -> int {
        const int dx = std::abs(x - target_x);
        const int dy = std::abs(y - target_y);
        return diagonals ? std::max(dx, dy) : (dx + dy);
    };

    std::priority_queue<SPathNode, std::vector<SPathNode>, std::greater<SPathNode>> open_set;

    grid[start.x * height + start.y] = {current_search_id, 0, start};
    open_set.push({start, 0, heuristic(static_cast<int>(start.x), static_cast<int>(start.y))});

    static constexpr std::array<std::pair<int, int>, 4> cardinalSteps = {{
        {0, -1}, {-1, 0}, {1, 0}, {0, 1}
    }};
    static constexpr std::array<std::pair<int, int>, 4> diagonalSteps = {{
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    }};

    while (!open_set.empty()) {
        const SPathNode current = open_set.top();
        open_set.pop();

        if (current.pos == end) {
            Position p = end;
            while (p != start) {
                path.push_back(p);
                p = grid[p.x * height + p.y].parent;
            }
            std::reverse(path.begin(), path.end());
            return true;
        }

        const std::size_t current_idx = current.pos.x * height + current.pos.y;
        if (current.g > grid[current_idx].g) {
            continue;
        }

        auto explore_step = [&](int dx, int dy) {
            const int nx = static_cast<int>(current.pos.x) + dx;
            const int ny = static_cast<int>(current.pos.y) + dy;

            if (nx < 0 || ny < 0 || nx >= static_cast<int>(width) || ny >= static_cast<int>(height)) {
                return;
            }

            const std::size_t unx = static_cast<std::size_t>(nx);
            const std::size_t uny = static_cast<std::size_t>(ny);

            if (level.get_unchecked(unx, uny).isPassable() != target_passable) {
                return;
            }

            const int tentative_g = current.g + 1;
            const std::size_t next_idx = unx * height + uny;
            auto& neighbor_cell = grid[next_idx];

            if (neighbor_cell.search_id != current_search_id || tentative_g < neighbor_cell.g) {
                neighbor_cell.search_id = current_search_id;
                neighbor_cell.g = tentative_g;
                neighbor_cell.parent = current.pos;

                const int h = heuristic(nx, ny);
                open_set.push({Position(unx, uny), tentative_g, tentative_g + h});
            }
        };

        // Straight cardinal steps evaluated first to favor straighter paths
        for (const auto& [dx, dy] : cardinalSteps) {
            explore_step(dx, dy);
        }
        if (diagonals) {
            for (const auto& [dx, dy] : diagonalSteps) {
                explore_step(dx, dy);
            }
        }
    }

    return false;
}

} // namespace RL

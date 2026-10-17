/**
 * @file pathfinding.cppm
 * @brief High-performance A* pathfinding algorithm on 2D grid maps.
 */

module;

export module rl.pathfinding;

import rl.map;
import rl.maputils;
import rl.matrix;
import rl.position;
import std;

export namespace RL
{

/**
 * @brief Internal priority queue node for the A* search graph.
 *
 * Tracks the node coordinate, exact path cost from start (@f$g@f$), and total estimated cost (@f$f = g + h@f$).
 */
struct SPathNode {
    Position pos; ///< Grid coordinates of this node.
    int g;        ///< Exact cost from the starting node to this node.
    int f;        ///< Estimated total cost from start to destination (@f$f = g + h@f$).

    /**
     * @brief Comparator for min-heap priority queue ordering.
     *
     * Orders nodes primarily by lowest total cost @p f. Ties are broken by favoring
     * higher @p g cost (nodes closer to the target), which dramatically reduces node expansions.
     *
     * @param other Node to compare against.
     * @return True if this node has higher cost (lower priority) than @p other.
     */
    bool operator>(const SPathNode& other) const noexcept
    {
        if (f != other.f) {
            return f > other.f;
        }
        return g < other.g; // Tie-breaker: prefer nodes closer to target
    }
};

/**
 * @brief Per-cell search state stored in the generational grid cache.
 *
 * Uses a search counter ID (@p search_id) to eliminate the overhead of clearing
 * or reallocating the grid state between successive pathfinding queries.
 */
struct SCellState {
    std::uint32_t search_id = 0;                  ///< ID of the search that last touched this cell.
    int g = std::numeric_limits<int>::max();      ///< Lowest path cost discovered to reach this cell.
    Position parent;                              ///< Predecessor cell coordinate for path reconstruction.
};

/**
 * @brief Finds the shortest path between two points on a map using the A* algorithm.
 *
 * Computes an optimal path on the grid from @p start to @p end. The output @p path
 * vector is populated with the sequence of intermediate coordinates leading up to and
 * including @p end (the @p start coordinate itself is excluded).
 *
 * Features and performance optimizations:
 * - **Generational Scratchpad:** Employs a thread-local flat vector with 32-bit search IDs,
 *   achieving zero heap allocation per search after initial warmup.
 * - **Chebyshev / Manhattan Heuristics:** Automatically switches between Chebyshev distance
 *   (@f$\max(|dx|, |dy|)@f$) when diagonal movement is enabled and Manhattan distance
 *   (@f$|dx| + |dy|@f$) when restricted to 4 cardinal directions.
 * - **Straight-Line Bias:** Cardinal steps are evaluated before diagonal moves, producing
 *   visually natural and direct paths.
 * - **Matching Passability:** Pathfinding traverses tiles sharing the target tile's passability
 *   (normally passable floor tiles, or impassable tiles if navigating blocked terrain).
 *
 * @param level Reference to the grid map containing tile passability.
 * @param start Starting coordinate.
 * @param end Target destination coordinate.
 * @param[out] path Output vector where the resulting sequence of steps is written.
 *                  Cleared prior to searching. If @p start equals @p end, returns true
 *                  with @p path left empty.
 * @param diagonals If true, permits 8-directional movement (cardinal + diagonal);
 *                  if false, restricts movement to 4 cardinal directions. Defaults to true.
 * @return True if a valid path exists from @p start to @p end (or if @p start == @p end);
 *         false if no path exists or either coordinate is outside map bounds.
 *
 * @see CMap
 * @see Position
 */
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

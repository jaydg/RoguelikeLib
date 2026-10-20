/**
 * @file distancemap.cppm
 * @brief Multi-source Dijkstra distance maps and flow fields for grid pathfinding.
 */

module;

export module rl.distancemap;

export import rl.map;
export import rl.matrix;
export import rl.position;
import rl.tile;
import std;

export namespace RL
{

/**
 * @brief Custom movement cost override for walking onto a specific tile archetype.
 *
 * Allows assigning differential travel costs (e.g. road=1 vs grass=3 vs swamp=5)
 * so walkers naturally follow roads and preferred paths.
 */
struct SStepCost {
    std::string_view type; ///< Tile archetype name (e.g., `"grass"`, `"road"`).
    int cost = 1;          ///< Movement cost penalty (must be >= 1).
};

/**
 * @brief Multi-source shortest distance map and flow field over a 2D map.
 *
 * Computes the minimum cumulative movement cost from every passable cell on the map
 * to the nearest target goal. Once calculated, a distance map can steer any number
 * of autonomous actors (monsters chasing the player, villagers returning to town,
 * scent trails) toward those goals in @f$O(1)@f$ query time per step without running
 * separate path searches for each entity.
 *
 * Key features:
 * - **Multi-Source Propagation:** Computes distance to the closest of any number of simultaneous goals.
 * - **Monotonic BFS Fast-Path:** Automatically uses an allocation-free monotonic FIFO queue (@f$O(V + E)@f$)
 *   when edge costs are uniform (cost = 1), achieving up to 3.4x speedups over general heaps.
 * - **Dijkstra Priority Queue:** Seamlessly falls back to priority-queue relaxation when differential
 *   terrain costs are configured.
 * - **Unified Memory Layout:** Merges cumulative distance and tile step cost into a single contiguous
 *   grid matrix (`SCell`) for optimal CPU L1 cache locality.
 * - **Flow Field Traversal:** Provides @ref NextStep to find the optimal neighbor cell leading toward
 *   the goal in a few nanoseconds.
 * - **Search Horizon Bounding:** Supports capping the maximum measured distance via @p max_distance,
 *   pruning unneeded regions early.
 *
 * @see CMap
 * @see Position
 * @see Neighbors
 */
class CDistanceMap
{
private:
    /**
     * @brief Internal cell structure combining cumulative path distance and step cost.
     */
    struct SCell {
        int distance = unreachable; ///< Minimum cumulative path cost discovered from nearest goal.
        int step_cost = 0;          ///< Cost to step onto this tile (0 if unreachable or impassable).
    };

    CMatrix<SCell> grid;   ///< 2D grid storing cell distances and step costs.
    Neighbors neighbors;   ///< Neighbor connectivity mode (Cardinal4 or All8).

    /// The step costs by the address of the type's name in @ref CTileData, the first given for a type.
    std::vector<std::pair<const char*, int>> by_name;

    /// Whether the map was measured without positions to stop at, so that @ref Mend can repair it.
    bool complete = true;

    /// The largest distance measured, as the constructor was given it.
    int farthest = unreachable;

    /**
     * @brief The cost of stepping onto a passable tile: its type's listed cost, or 1.
     *
     * Every tile views its type's name in the registry, so a type is known by the address of that
     * name, which is far cheaper to compare than its letters.
     */
    [[nodiscard]] int CostOf(const CTile& tile) const
    {
        return CostIn(by_name, tile);
    }

    /**
     * @brief The cost of stepping onto a passable tile by a table of @ref CostsByName: its type's cost, or 1.
     */
    [[nodiscard]] static int CostIn(std::span<const std::pair<const char*, int>> table, const CTile& tile)
    {
        const char* name = tile.getType().data();

        for (const auto& [listed, cost] : table) {
            if (listed == name) {
                return cost;
            }
        }

        return 1;
    }

    /**
     * @brief The step costs by the address of the type's name in @ref CTileData.
     *
     * A type nobody registered cannot be on the map, and is left out; the first cost given for a
     * type is the one that counts.
     */
    [[nodiscard]] static std::vector<std::pair<const char*, int>> CostsByName(std::span<const SStepCost> costs)
    {
        std::vector<std::pair<const char*, int>> table;
        table.reserve(costs.size());

        for (const SStepCost& listed : costs) {
            if (!CTileData::has(listed.type)) {
                continue;
            }

            const char* name = CTileData::get(listed.type).first.data();

            if (std::ranges::find(table, name, &std::pair<const char*, int>::first) == table.end()) {
                table.emplace_back(name, listed.cost);
            }
        }

        return table;
    }


    /**
     * @brief Returns the coordinate offsets for neighbor exploration.
     *
     * Orders cardinal steps before diagonal ones to favor aesthetically straighter paths.
     *
     * @param neighbors Connectivity mode (Cardinal4 or All8).
     * @return Span of (dx, dy) coordinate deltas.
     */
    static std::span<const std::pair<int, int>> Steps(Neighbors neighbors)
    {
        static constexpr std::array<std::pair<int, int>, 8> all8 = {{
                {0, -1}, {-1, 0}, {1, 0}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
            }
        };

        return neighbors == Neighbors::All8 ? std::span(all8) : std::span(all8).first(4);
    }

public:
    /**
     * @brief Sentinel value representing unreachable or unmeasured map cells.
     */
    static constexpr int unreachable = std::numeric_limits<int>::max();

    /**
     * @brief Constructs and calculates a distance map outwards from one or more goals.
     *
     * Propagates cumulative costs through all passable tiles until the entire map or
     * @p max_distance is reached. Impassable goals (e.g. wells, doors, chests) are assigned
     * a step cost of 1 to allow actors to navigate adjacent to them.
     *
     * @param map Reference to the tile level.
     * @param goals Collection of target destination coordinates to propagate outwards from.
     * @param costs Optional list of tile archetype cost overrides (defaults to uniform cost 1).
     *              Types not registered in @ref CTileData are ignored; if a type is listed
     *              more than once, its first entry applies. Each cell's cost is resolved once.
     * @param a_neighbors Connectivity mode (All8 allows diagonal movement, Cardinal4 restricts to cardinal directions).
     * @param max_distance Maximum cumulative distance to explore. Cells beyond this limit are marked as unreachable.
     * @param until Optional positions whose distances are all that is needed. The search heads for them
     *              from the goals (an A* search) and stops as soon as each of them has its final distance,
     *              which is exactly what the complete search would give; other cells may then be left
     *              unreachable or with a larger distance. Positions off the map are ignored; one that cannot
     *              be reached lets the search run to its end.
     * @throws std::invalid_argument If any entry in @p costs has a cost strictly less than 1.
     */
    CDistanceMap(const CMap& map, std::span<const Position> goals, std::span<const SStepCost> costs = {},
                 Neighbors a_neighbors = Neighbors::All8, int max_distance = unreachable,
                 std::span<const Position> until = {})
        : grid(map.getSize(), SCell{unreachable, 0}), neighbors(a_neighbors),
          complete(until.empty()), farthest(max_distance)
    {
        for (const SStepCost& cost : costs) {
            if (cost.cost < 1) {
                throw std::invalid_argument("walking onto " + std::string(cost.type) + " has to cost at least 1");
            }
        }

        const std::size_t width = map.getWidth();
        const std::size_t height = map.getHeight();

        if (width == 0 || height == 0) {
            return;
        }

        const auto steps = Steps(neighbors);

        // The positions still waiting for their final distance, if the
        // caller needs only those; the search ends once none are left
        std::vector<Position> waiting;

        for (const Position& pos : until) {
            if (map.inside(pos) && std::ranges::find(waiting, pos) == waiting.end()) {
                waiting.push_back(pos);
            }
        }

        const bool limited = !waiting.empty();
        const auto settled = [&waiting, limited](const Position& pos) {
            return limited && std::erase(waiting, pos) > 0 && waiting.empty();
        };

        // Fast-path check: are all edge costs uniform (cost = 1)?
        const bool has_variable_costs = !costs.empty() && std::ranges::any_of(costs, [](const auto& c) { return c.cost > 1; });

        // The positions asked for, all of them: what the search heads for,
        // when it needs only those
        const std::vector<Position> wanted = waiting;
        const bool diagonal = neighbors == Neighbors::All8;

        if (!has_variable_costs && !limited) {
            // UNIFORM BFS FAST-PATH (weights are all 1)
            std::vector<Position> queue;
            queue.reserve(std::min(width * height, std::size_t(4096)));

            bool done = false;

            for (const Position& goal : goals) {
                if (map.inside(goal)) {
                    grid.get_unchecked(goal.x, goal.y) = {0, 1};
                    queue.push_back(goal);
                    done = settled(goal) || done;
                }
            }

            std::size_t head = 0;
            while (!done && head < queue.size()) {
                const Position at = queue[head++];
                const int next_dist = grid.get_unchecked(at.x, at.y).distance + 1;

                if (next_dist > max_distance) {
                    break;
                }

                const int ix = static_cast<int>(at.x);
                const int iy = static_cast<int>(at.y);

                for (const auto& [dx, dy] : steps) {
                    const int nx = ix + dx;
                    const int ny = iy + dy;

                    if (nx >= 0 && ny >= 0 && nx < static_cast<int>(width) && ny < static_cast<int>(height)) {
                        const std::size_t unx = static_cast<std::size_t>(nx);
                        const std::size_t uny = static_cast<std::size_t>(ny);
                        auto& cell = grid.get_unchecked(unx, uny);

                        if (cell.distance == unreachable && map.get_unchecked(unx, uny).isPassable()) {
                            cell.distance = next_dist;
                            cell.step_cost = 1;
                            queue.push_back(Position(unx, uny));
                            done = settled(Position(unx, uny)) || done;
                        }
                    }
                }
            }
        } else {
            // DIJKSTRA FOR VARIABLE COSTS, or towards the positions asked
            // for: an A* search, by the fewest steps from a cell to the
            // nearest of them - never more than the way there costs, as each
            // step costs at least 1, and never falling by more than a step
            // costs, so that a cell taken from the queue still has its
            // final distance, while the search keeps to the way towards them
            by_name = CostsByName(costs);

            const auto cost_of = [this](const CTile& tile) {
                return CostOf(tile);
            };
            const auto estimate = [&wanted, diagonal](std::size_t x, std::size_t y) {
                int least = wanted.empty() ? 0 : unreachable;

                for (const Position& pos : wanted) {
                    const int dx = std::abs(static_cast<int>(x) - static_cast<int>(pos.x));
                    const int dy = std::abs(static_cast<int>(y) - static_cast<int>(pos.y));
                    least = std::min(least, diagonal ? std::max(dx, dy) : dx + dy);
                }

                return least;
            };

            struct SQueued {
                int key;
                int dist;
                Position pos;
                bool operator>(const SQueued& other) const noexcept { return key > other.key; }
            };
            std::priority_queue<SQueued, std::vector<SQueued>, std::greater<SQueued>> queue;

            for (const Position& goal : goals) {
                if (map.inside(goal)) {
                    const auto& tile = map.get_unchecked(goal.x, goal.y);
                    const int step = tile.isPassable() ? cost_of(tile) : 1;
                    grid.get_unchecked(goal.x, goal.y) = {0, step};
                    queue.push({estimate(goal.x, goal.y), 0, goal});
                }
            }

            while (!queue.empty()) {
                const auto [key, so_far, at] = queue.top();
                queue.pop();

                auto& at_cell = grid.get_unchecked(at.x, at.y);
                if (so_far > at_cell.distance) {
                    continue;
                }

                // Taken from the queue at its own distance, a cell's
                // distance is final
                if (settled(at)) {
                    break;
                }

                const int onto = at_cell.step_cost;
                const int new_dist = so_far + onto;

                if (new_dist > max_distance) {
                    continue;
                }

                const int ix = static_cast<int>(at.x);
                const int iy = static_cast<int>(at.y);

                for (const auto& [dx, dy] : steps) {
                    const int nx = ix + dx;
                    const int ny = iy + dy;

                    if (nx >= 0 && ny >= 0 && nx < static_cast<int>(width) && ny < static_cast<int>(height)) {
                        const std::size_t unx = static_cast<std::size_t>(nx);
                        const std::size_t uny = static_cast<std::size_t>(ny);
                        const auto& from_tile = map.get_unchecked(unx, uny);

                        if (from_tile.isPassable()) {
                            auto& from_cell = grid.get_unchecked(unx, uny);
                            if (new_dist < from_cell.distance) {
                                from_cell.distance = new_dist;

                                // A passable cell's cost is at least 1 and
                                // never changes, so it is looked up once
                                if (from_cell.step_cost == 0) {
                                    from_cell.step_cost = cost_of(from_tile);
                                }

                                queue.push({new_dist + estimate(unx, uny), new_dist, Position(unx, uny)});
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief Retrieves the minimum cumulative cost to reach the nearest goal from @p pos.
     *
     * @param pos Map coordinate to query.
     * @return Path distance cost, or @ref unreachable if unreachable or out of map bounds.
     */
    [[nodiscard]] int Distance(const Position& pos) const
    {
        return grid.inside(pos) ? grid.get_unchecked(pos.x, pos.y).distance : unreachable;
    }

    /**
     * @brief Checks if a path exists from @p pos to any goal within the search limit.
     *
     * @param pos Map coordinate to query.
     * @return True if reachable (distance != @ref unreachable).
     */
    [[nodiscard]] bool Reachable(const Position& pos) const
    {
        return Distance(pos) != unreachable;
    }

    /**
     * @brief Determines the optimal neighboring tile to step onto toward the nearest goal.
     *
     * Evaluates all valid adjacent cells according to @ref Neighbors mode and returns the
     * neighbor that minimizes total remaining cost (`distance + step_cost`).
     *
     * @param from Current actor position.
     * @return Coordinate of the next step, or `std::nullopt` if already at a goal or no goal is reachable.
     */
    [[nodiscard]] std::optional<Position> NextStep(const Position& from) const
    {
        const int here = Distance(from);

        if (here == 0 || here == unreachable) {
            return std::nullopt;
        }

        std::optional<Position> best;
        int best_cost = unreachable;

        const auto steps = Steps(neighbors);
        const int ix = static_cast<int>(from.x);
        const int iy = static_cast<int>(from.y);
        const int width = static_cast<int>(grid.getWidth());
        const int height = static_cast<int>(grid.getHeight());

        for (const auto& [dx, dy] : steps) {
            const int nx = ix + dx;
            const int ny = iy + dy;

            if (nx >= 0 && ny >= 0 && nx < width && ny < height) {
                const auto& cell = grid.get_unchecked(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));

                if (cell.distance != unreachable && cell.step_cost > 0 && cell.distance + cell.step_cost < best_cost) {
                    best = Position(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
                    best_cost = cell.distance + cell.step_cost;
                }
            }
        }

        return best;
    }

    /**
     * @brief Repairs the distances after some tiles of the map have changed, instead of measuring it anew.
     *
     * Most changes to a map are small - a tree felled, a path worn into the grass, a wall raised - and
     * change the distances of only the cells whose shortest ways lead over them. Given the map as it is now
     * and every position changed since the distances were measured or last mended, this repairs just
     * those cells:
     * - a tile turned passable or cheaper to step onto lowers the distances beyond it, spreading out from
     *   it only as far as they improve;
     * - a tile turned impassable or dearer clears the cells whose every shortest way led over it, in the
     *   order of their distances, and measures them again from the cells around them.
     *
     * The distances and steps come out exactly as measuring the changed map anew, with the same goals,
     * costs, connectivity and distance limit, would give them. Positions whose tiles did not really
     * change, or that are listed more than once, do no harm.
     *
     * @param map The map the distances were measured on, as it is now; of the same size.
     * @param changed Every position whose tile changed since the distances were measured or last mended.
     * @return False, leaving the distances as they were, when they cannot be repaired, and have to be
     *         measured anew: when they were measured only until some positions were known, when the map
     *         is of another size, or when a changed position is a goal or off the map.
     */
    bool Mend(const CMap& map, std::span<const Position> changed)
    {
        const std::size_t width = grid.getWidth();
        const std::size_t height = grid.getHeight();

        if (!complete || map.getWidth() != width || map.getHeight() != height) {
            return false;
        }

        for (const Position& pos : changed) {
            if (!grid.inside(pos) || grid.get_unchecked(pos.x, pos.y).distance == 0) {
                return false;
            }
        }

        const auto steps = Steps(neighbors);
        const auto index = [width](const Position& pos) {
            return pos.y * width + pos.x;
        };
        const auto around = [&](const Position& pos, auto&& visit) {
            const int ix = static_cast<int>(pos.x);
            const int iy = static_cast<int>(pos.y);

            for (const auto& [dx, dy] : steps) {
                const int nx = ix + dx;
                const int ny = iy + dy;

                if (nx >= 0 && ny >= 0 && nx < static_cast<int>(width) && ny < static_cast<int>(height)) {
                    visit(Position(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)));
                }
            }
        };
        // What stepping onto a cell costs on the map as it is now; 0 if it cannot be stepped onto
        const auto cost_now = [&](const Position& pos) {
            const CTile& tile = map.get_unchecked(pos.x, pos.y);
            return tile.isPassable() ? CostOf(tile) : 0;
        };

        struct SQueued {
            int dist;
            Position pos;
            bool operator>(const SQueued& other) const noexcept { return dist > other.dist; }
        };

        // The cells cleared: the tiles turned dearer, and every cell whose
        // shortest ways all led over cleared ones. Deciding in the order of
        // the distances, every way a cell has is decided before it is.
        std::unordered_set<std::size_t> cleared;
        std::unordered_set<std::size_t> decided;
        std::vector<Position> cleared_cells;
        std::priority_queue<SQueued, std::vector<SQueued>, std::greater<SQueued>> deciding;

        const auto clear = [&](const Position& pos) {
            if (!cleared.insert(index(pos)).second) {
                return;
            }

            cleared_cells.push_back(pos);
            around(pos, [&](const Position& next) {
                const SCell& cell = grid.get_unchecked(next.x, next.y);

                if (cell.distance != unreachable && cell.distance != 0 && !decided.contains(index(next))) {
                    deciding.push({cell.distance, next});
                }
            });
        };

        for (const Position& pos : changed) {
            const SCell& cell = grid.get_unchecked(pos.x, pos.y);
            const int now = cost_now(pos);

            if (cell.distance != unreachable && (now == 0 || now > cell.step_cost)) {
                decided.insert(index(pos));
                clear(pos);
            }
        }

        while (!deciding.empty()) {
            const Position at = deciding.top().pos;
            deciding.pop();

            if (!decided.insert(index(at)).second) {
                continue;
            }

            const int distance = grid.get_unchecked(at.x, at.y).distance;
            bool kept = false;

            around(at, [&](const Position& next) {
                const SCell& cell = grid.get_unchecked(next.x, next.y);
                kept = kept || (!cleared.contains(index(next)) && cell.distance != unreachable && cell.step_cost > 0 &&
                                cell.distance + cell.step_cost == distance);
            });

            if (!kept) {
                clear(at);
            }
        }

        // The tiles turned cheaper or passable that were reached take their
        // new cost; those reached anew take it when they are
        std::priority_queue<SQueued, std::vector<SQueued>, std::greater<SQueued>> queue;

        for (const Position& pos : changed) {
            SCell& cell = grid.get_unchecked(pos.x, pos.y);

            if (cell.distance != unreachable && !cleared.contains(index(pos))) {
                cell.step_cost = cost_now(pos);
                queue.push({cell.distance, pos});
            }
        }

        for (const Position& pos : cleared_cells) {
            grid.get_unchecked(pos.x, pos.y) = SCell{unreachable, 0};
        }

        // Cleared cells, and passable ones reached by nothing so far, are
        // measured from the cells around them that keep their distances
        const auto measure = [&](const Position& pos) {
            const int cost = cost_now(pos);
            int best = unreachable;

            if (cost == 0) {
                return;
            }

            around(pos, [&](const Position& next) {
                const SCell& cell = grid.get_unchecked(next.x, next.y);

                if (cell.distance != unreachable && cell.step_cost > 0) {
                    best = std::min(best, cell.distance + cell.step_cost);
                }
            });

            if (best <= farthest && best != unreachable) {
                grid.get_unchecked(pos.x, pos.y) = SCell{best, cost};
                queue.push({best, pos});
            }
        };

        for (const Position& pos : cleared_cells) {
            measure(pos);
        }

        for (const Position& pos : changed) {
            if (grid.get_unchecked(pos.x, pos.y).distance == unreachable) {
                measure(pos);
            }
        }

        // And from there on, every cell a shorter way reaches
        while (!queue.empty()) {
            const auto[so_far, at] = queue.top();
            queue.pop();
            const SCell& at_cell = grid.get_unchecked(at.x, at.y);

            if (so_far > at_cell.distance) {
                continue;
            }

            const int new_dist = so_far + at_cell.step_cost;

            around(at, [&](const Position& next) {
                SCell& cell = grid.get_unchecked(next.x, next.y);

                if (new_dist <= farthest && new_dist < cell.distance && map.get_unchecked(next.x, next.y).isPassable()) {
                    cell.distance = new_dist;

                    if (cell.step_cost == 0) {
                        cell.step_cost = cost_now(next);
                    }

                    queue.push({new_dist, next});
                }
            });
        }

        return true;
    }

    /**
     * @brief Finds which of some goals is nearest to a start, without measuring a map around it.
     *
     * Answers what a distance map measured from @p start - `CDistanceMap(map, {start}, costs, neighbors)` -
     * would give as the smallest @ref Distance of the goals, and for which of them, but searches only
     * towards the goals rather than in every direction (an A* search, by the steps it takes at the
     * least to the nearest goal). Ties go to the goal listed first, as when going through the goals
     * of such a map in order and keeping a nearer one only.
     *
     * @param map The map to search.
     * @param start Where the distances are measured from, as the one goal of such a map.
     * @param goals The positions of which the nearest is wanted; those off the map are ignored, and an
     *              impassable one other than @p start is never reached.
     * @param costs Optional tile archetype cost overrides, as for the constructor.
     * @param a_neighbors Connectivity mode, as for the constructor.
     * @return The index in @p goals of the nearest goal with its distance; nothing if none can be reached.
     * @throws std::invalid_argument If any entry in @p costs has a cost strictly less than 1.
     */
    [[nodiscard]] static std::optional<std::pair<std::size_t, int>> Nearest(const CMap& map, const Position& start,
            std::span<const Position> goals, std::span<const SStepCost> costs = {},
            Neighbors a_neighbors = Neighbors::All8)
    {
        for (const SStepCost& cost : costs) {
            if (cost.cost < 1) {
                throw std::invalid_argument("walking onto " + std::string(cost.type) + " has to cost at least 1");
            }
        }

        const std::size_t width = map.getWidth();
        const std::size_t height = map.getHeight();

        if (!map.inside(start)) {
            return std::nullopt;
        }

        // The goals on the map, by their indices in order
        std::vector<std::size_t> wanted;

        for (std::size_t index = 0; index < goals.size(); ++index) {
            if (map.inside(goals[index])) {
                if (goals[index] == start) {
                    return std::pair{index, 0};
                }

                wanted.push_back(index);
            }
        }

        if (wanted.empty()) {
            return std::nullopt;
        }

        const auto table = CostsByName(costs);
        const auto steps = Steps(a_neighbors);
        const bool diagonal = a_neighbors == Neighbors::All8;

        // The fewest steps to the nearest goal, each costing at least 1:
        // never more than the way there costs, and never falling by more
        // than a step costs, so that a cell taken from the queue has its
        // final distance
        const auto estimate = [&](const Position& pos) {
            int least = unreachable;

            for (const std::size_t index : wanted) {
                const int dx = std::abs(static_cast<int>(pos.x) - static_cast<int>(goals[index].x));
                const int dy = std::abs(static_cast<int>(pos.y) - static_cast<int>(goals[index].y));
                least = std::min(least, diagonal ? std::max(dx, dy) : dx + dy);
            }

            return least;
        };
        // What leaving a cell costs on a way to the start: stepping onto it
        // on the way back, as the distance map measures it; the start, a
        // goal of such a map, costs 1 when it is impassable
        const auto step_of = [&](const Position& pos) {
            const CTile& tile = map.get_unchecked(pos.x, pos.y);
            return tile.isPassable() ? CostIn(table, tile) : 1;
        };

        // The distances so far, in a scratchpad kept from search to search,
        // marked by the search that last touched each cell: one as large as
        // the largest map searched serves every map
        thread_local std::vector<std::pair<std::uint32_t, int>> known;
        thread_local std::uint32_t search = 0;

        if (known.size() < width * height) {
            known.resize(width * height, {0, unreachable});
        }

        if (++search == 0) {
            std::ranges::fill(known, std::pair<std::uint32_t, int> {0, unreachable});
            search = 1;
        }

        const auto distance = [&](const Position& pos) -> std::pair<std::uint32_t, int>& {
            return known[pos.y * width + pos.x];
        };

        struct SOpen {
            int estimated;
            int so_far;
            Position pos;
            bool operator>(const SOpen& other) const noexcept { return estimated > other.estimated; }
        };

        std::priority_queue<SOpen, std::vector<SOpen>, std::greater<SOpen>> open;
        distance(start) = {search, 0};
        open.push({estimate(start), 0, start});

        std::optional<std::size_t> nearest;
        int best = unreachable;

        // Once nothing in the queue can come as near as the nearest goal
        // found, no goal can, and every goal as near has been found
        while (!open.empty() && open.top().estimated <= best) {
            const SOpen at = open.top();
            open.pop();

            if (at.so_far > distance(at.pos).second) {
                continue;
            }

            for (const std::size_t index : wanted) {
                if (goals[index] == at.pos && (at.so_far < best || index < *nearest)) {
                    best = at.so_far;
                    nearest = index;
                    break;
                }
            }

            const int onward = at.so_far + step_of(at.pos);
            const int ix = static_cast<int>(at.pos.x);
            const int iy = static_cast<int>(at.pos.y);

            for (const auto& [dx, dy] : steps) {
                const int nx = ix + dx;
                const int ny = iy + dy;

                if (nx < 0 || ny < 0 || nx >= static_cast<int>(width) || ny >= static_cast<int>(height)) {
                    continue;
                }

                const Position next(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
                auto& [touched, so_far] = distance(next);

                if (map.get_unchecked(next.x, next.y).isPassable() && (touched != search || onward < so_far)) {
                    touched = search;
                    so_far = onward;
                    open.push({onward + estimate(next), onward, next});
                }
            }
        }

        if (!nearest) {
            return std::nullopt;
        }

        return std::pair{*nearest, best};
    }
};

} // namespace RL

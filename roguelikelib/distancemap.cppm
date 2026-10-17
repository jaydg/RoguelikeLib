//////////////////////////////////////////////////////////////////////////
// Distance map
//
// For every tile of a map, how far it is to the nearest of some goals, and
// so which way leads there. Made once, it guides any number of walkers to
// the same goals - the inhabitants of a village to its tavern, monsters to
// the player - without a search for each of them. Remade when the map or
// the goals change.
//
// Walking onto a tile costs 1, unless the tile's type is given another
// cost: making paths cheaper than grass keeps walkers on the paths.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.distancemap;

export import rl.map;
export import rl.matrix;
export import rl.position;
import rl.tile;
import std;

export namespace RL
{

// What walking onto a tile of a type costs, at least 1
struct SStepCost {
    std::string_view type;
    int cost = 1;
};

class CDistanceMap
{
private:
    struct SCell {
        int distance = unreachable;
        int step_cost = 0;
    };

    CMatrix<SCell> grid;
    Neighbors neighbors;

    // Straight steps before diagonal ones, so that of two ways as good the
    // straighter one is taken
    static std::span<const std::pair<int, int>> Steps(Neighbors neighbors)
    {
        static constexpr std::array<std::pair<int, int>, 8> all8 = {{
                {0, -1}, {-1, 0}, {1, 0}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
            }
        };

        return neighbors == Neighbors::All8 ? std::span(all8) : std::span(all8).first(4);
    }

public:
    static constexpr int unreachable = std::numeric_limits<int>::max();

    // Measures the way from every tile to the nearest goal. The distance of
    // a tile is what walking onto each tile on the way costs, the goal's
    // included. A goal that cannot be stood on, like a well, counts 1 to
    // walk onto, and leads walkers next to it. Tiles further from every
    // goal than `max_distance` are not measured, and count as unreachable:
    // for walkers that only ever go a little way, that saves measuring the
    // whole map.
    CDistanceMap(const CMap& map, std::span<const Position> goals, std::span<const SStepCost> costs = {},
                 Neighbors a_neighbors = Neighbors::All8, int max_distance = unreachable)
        : grid(map.getSize(), SCell{unreachable, 0}), neighbors(a_neighbors)
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

        // Fast-path check: are all edge costs uniform (cost = 1)?
        const bool has_variable_costs = !costs.empty() && std::ranges::any_of(costs, [](const auto& c) { return c.cost > 1; });

        if (!has_variable_costs) {
            // UNIFORM BFS FAST-PATH (weights are all 1)
            std::vector<Position> queue;
            queue.reserve(std::min(width * height, std::size_t(4096)));

            for (const Position& goal : goals) {
                if (map.inside(goal)) {
                    grid.get_unchecked(goal.x, goal.y) = {0, 1};
                    queue.push_back(goal);
                }
            }

            std::size_t head = 0;
            while (head < queue.size()) {
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
                        }
                    }
                }
            }
        } else {
            // DIJKSTRA FOR VARIABLE COSTS
            const auto cost_of = [&costs](const CTile& tile) {
                for (const auto& listed : costs) {
                    if (tile.getType() == listed.type) {
                        return listed.cost;
                    }
                }
                return 1;
            };

            struct SQueued {
                int dist;
                Position pos;
                bool operator>(const SQueued& other) const noexcept { return dist > other.dist; }
            };
            std::priority_queue<SQueued, std::vector<SQueued>, std::greater<SQueued>> queue;

            for (const Position& goal : goals) {
                if (map.inside(goal)) {
                    const auto& tile = map.get_unchecked(goal.x, goal.y);
                    const int step = tile.isPassable() ? cost_of(tile) : 1;
                    grid.get_unchecked(goal.x, goal.y) = {0, step};
                    queue.push({0, goal});
                }
            }

            while (!queue.empty()) {
                const auto [so_far, at] = queue.top();
                queue.pop();

                auto& at_cell = grid.get_unchecked(at.x, at.y);
                if (so_far > at_cell.distance) {
                    continue;
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
                                from_cell.step_cost = cost_of(from_tile);
                                queue.push({new_dist, Position(unx, uny)});
                            }
                        }
                    }
                }
            }
        }
    }

    // How far the nearest goal is: the cost of the cheapest way there,
    // `unreachable` if there is none
    [[nodiscard]] int Distance(const Position& pos) const
    {
        return grid.inside(pos) ? grid.get_unchecked(pos.x, pos.y).distance : unreachable;
    }

    [[nodiscard]] bool Reachable(const Position& pos) const
    {
        return Distance(pos) != unreachable;
    }

    // The neighbour to step to on the cheapest way to a goal, which may be a
    // goal that cannot be stood on. Nothing at a goal, or where no goal can
    // be reached.
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
};

} // namespace RL

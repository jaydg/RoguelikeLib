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
    CMatrix<int> distance;

    // What walking onto each tile costs, for those the measuring reached;
    // 0 where it cannot be done, or was not measured
    CMatrix<int> step_cost;

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

    // The neighbours of a position that lie on the map
    template <typename Visit>
    void ForNeighbors(const Position& at, Visit&& visit) const
    {
        for (const auto& [dx, dy] : Steps(neighbors)) {
            const auto x = static_cast<std::int64_t>(at.x) + dx;
            const auto y = static_cast<std::int64_t>(at.y) + dy;

            if (x >= 0 && y >= 0 && distance.inside(static_cast<std::size_t>(x), static_cast<std::size_t>(y))) {
                visit(Position(static_cast<std::size_t>(x), static_cast<std::size_t>(y)));
            }
        }
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
        : distance(map.getSize(), unreachable), step_cost(map.getSize(), 0), neighbors(a_neighbors)
    {
        for (const SStepCost& cost : costs) {
            if (cost.cost < 1) {
                throw std::invalid_argument("walking onto " + std::string(cost.type) + " has to cost at least 1");
            }
        }

        // What walking onto a tile that can be stood on costs
        const auto cost_of = [&costs](const CTile & tile) {
            for (const SStepCost& listed : costs) {
                if (tile.getType() == listed.type) {
                    return listed.cost;
                }
            }

            return 1;
        };

        // From the goals outwards: a tile is as far as the cheapest
        // neighbour, plus what walking onto that neighbour costs
        using SQueued = std::pair<int, Position>;
        const auto later = [](const SQueued & a, const SQueued & b) {
            return a.first > b.first;
        };
        std::priority_queue<SQueued, std::vector<SQueued>, decltype(later)> queue(later);

        for (const Position& goal : goals) {
            if (map.inside(goal)) {
                distance.set(goal, 0);
                step_cost.set(goal, map.get(goal).isPassable() ? cost_of(map.get(goal)) : 1);
                queue.emplace(0, goal);
            }
        }

        while (!queue.empty()) {
            const auto[so_far, at] = queue.top();
            queue.pop();

            if (so_far > distance.get(at)) {
                continue;
            }

            const int onto = step_cost.get(at);

            // Only tiles that can be stood on lead anywhere
            ForNeighbors(at, [&](const Position & from) {
                if (map.get(from).isPassable() && so_far + onto < distance.get(from) && so_far + onto <= max_distance) {
                    distance.set(from, so_far + onto);
                    step_cost.set(from, cost_of(map.get(from)));
                    queue.emplace(so_far + onto, from);
                }
            });
        }
    }

    // How far the nearest goal is: the cost of the cheapest way there,
    // `unreachable` if there is none
    [[nodiscard]] int Distance(const Position& pos) const
    {
        return distance.inside(pos) ? distance.get(pos) : unreachable;
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

        ForNeighbors(from, [&](const Position & next) {
            const int there = distance.get(next);
            const int onto = step_cost.get(next);

            if (there != unreachable && onto > 0 && there + onto < best_cost) {
                best = next;
                best_cost = there + onto;
            }
        });

        return best;
    }
};

} // namespace RL

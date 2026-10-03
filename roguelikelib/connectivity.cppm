//////////////////////////////////////////////////////////////////////////
// Connectivity
//
// Which parts of a map can be walked between, and making every walkable
// tile reachable from every other one.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.connectivity;

export import rl.map;
export import rl.matrix;
export import rl.position;
import rl.tile;
import std;

namespace RL
{

// The steps from a tile to its neighbours
std::span<const std::pair<int, int>> Steps(Neighbors neighbors)
{
    static constexpr std::array<std::pair<int, int>, 8> all8 = {{
            {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
        }
    };
    static constexpr std::array<std::pair<int, int>, 4> cardinal4 = {{
            {0, -1}, {-1, 0}, {1, 0}, {0, 1}
        }
    };

    if (neighbors == Neighbors::All8) {
        return all8;
    }

    return cardinal4;
}

} // namespace RL

export namespace RL
{

// The areas of a map that can be walked between
struct SRegions {
    // The region of each tile, -1 where it cannot be walked on
    CMatrix<int> region;

    // How many tiles each region has
    std::vector<std::size_t> sizes;

    // The region with the most tiles, the first of them if several are as
    // large; -1 when nothing can be walked on
    [[nodiscard]] int Largest() const
    {
        if (sizes.empty()) {
            return -1;
        }

        return static_cast<int>(std::ranges::max_element(sizes) - sizes.begin());
    }
};

// Finds the regions of a map. With Neighbors::All8 a diagonal step joins
// two tiles, with Neighbors::Cardinal4 it does not.
SRegions FindRegions(const CMap& map, Neighbors neighbors = Neighbors::All8)
{
    SRegions result{CMatrix<int>(map.getSize(), -1), {}};
    const auto width = static_cast<int>(map.getWidth());
    const auto height = static_cast<int>(map.getHeight());
    std::vector<Position> stack;

    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            if (result.region.get(x, y) != -1 || !map.get(x, y).isPassable()) {
                continue;
            }

            const int id = static_cast<int>(result.sizes.size());
            std::size_t count = 0;
            result.region.set(x, y, id);
            stack.emplace_back(x, y);

            while (!stack.empty()) {
                const Position at = stack.back();
                stack.pop_back();
                ++count;

                for (const auto& [dx, dy] : Steps(neighbors)) {
                    const int nx = static_cast<int>(at.x) + dx;
                    const int ny = static_cast<int>(at.y) + dy;

                    if (nx >= 0 && ny >= 0 && nx < width && ny < height && result.region.get(nx, ny) == -1 &&
                            map.get(nx, ny).isPassable()) {
                        result.region.set(nx, ny, id);
                        stack.emplace_back(nx, ny);
                    }
                }
            }

            result.sizes.push_back(count);
        }
    }

    return result;
}

// How a tile type that is in the way can be opened up
struct SOpening {
    // The tile type
    std::string_view type;

    // What opening one such tile costs, at least 1. The way chosen is the
    // one that costs least in all.
    int cost = 1;

    // The walkable tile type it becomes
    std::string_view becomes;
};

struct SReachability {
    // Regions joined to the largest one
    std::size_t regions_joined = 0;

    // How many tiles of each type were opened up
    std::map<std::string, std::size_t, std::less<>> opened;

    // Regions there is no way to, walled in by what may not be opened
    std::size_t regions_unreachable = 0;
};

// Joins every region of the map to the largest one, along the way that
// costs least to open. Only tile types listed in `openings` are ever
// opened; anything else that cannot be walked on stays as it is. Uses no
// random numbers, so the same map is always joined the same way.
//
// For a forest: RL::MakeReachable(map, RL::ForestOpenings)
SReachability MakeReachable(CMap& map, std::span<const SOpening> openings, Neighbors neighbors = Neighbors::All8)
{
    for (const SOpening& opening : openings) {
        if (opening.cost < 1) {
            throw std::invalid_argument("opening " + std::string(opening.type) + " has to cost at least 1");
        }
    }

    SReachability result;
    const SRegions regions = FindRegions(map, neighbors);
    const int largest = regions.Largest();

    if (largest < 0) {
        return result;
    }

    constexpr int closed = -1;
    constexpr int unknown = std::numeric_limits<int>::max();

    const std::size_t width = map.getWidth();
    const std::size_t height = map.getHeight();

    // What opening a tile costs: 0 if it is walkable already
    const auto opening_cost = [&map, &openings](std::size_t x, std::size_t y) {
        const CTile& tile = map.get(x, y);

        if (tile.isPassable()) {
            return 0;
        }

        for (const SOpening& opening : openings) {
            if (tile.getType() == opening.type) {
                return opening.cost;
            }
        }

        return closed;
    };

    // The tiles that can be reached from the largest region so far, and
    // the tiles of every region
    std::vector<bool> reached(width * height, false);
    std::vector<std::vector<std::size_t>> members(regions.sizes.size());

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (const int region = regions.region.get(x, y); region >= 0) {
                members[static_cast<std::size_t>(region)].push_back(y * width + x);
                reached[y * width + x] = region == largest;
            }
        }
    }

    // The largest regions first: joining them may join smaller ones on the
    // way
    std::vector<std::size_t> order(members.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(order, std::greater<>(), [&members](std::size_t region) {
        return members[region].size();
    });

    // Costs start out unknown, and after each search only the tiles it
    // touched are reset, so that many small regions stay cheap to join
    std::vector<int> cost(width * height, unknown);
    std::vector<std::size_t> from(width * height);
    std::vector<std::size_t> touched;
    using SQueued = std::pair<int, std::size_t>;

    for (const std::size_t region : order) {
        if (static_cast<int>(region) == largest) {
            continue;
        }

        // Joined already by the way opened for another region
        if (reached[members[region].front()]) {
            ++result.regions_joined;
            continue;
        }

        for (const std::size_t tile : touched) {
            cost[tile] = unknown;
        }

        touched.clear();

        // The cheapest way from the region to anything reached already
        std::priority_queue<SQueued, std::vector<SQueued>, std::greater<>> queue;

        for (const std::size_t tile : members[region]) {
            cost[tile] = 0;
            from[tile] = tile;
            touched.push_back(tile);
            queue.emplace(0, tile);
        }

        std::optional<std::size_t> goal;

        while (!queue.empty()) {
            const auto[so_far, tile] = queue.top();
            queue.pop();

            if (so_far > cost[tile]) {
                continue;
            }

            if (reached[tile]) {
                goal = tile;
                break;
            }

            for (const auto& [dx, dy] : Steps(neighbors)) {
                const auto nx = static_cast<std::int64_t>(tile % width) + dx;
                const auto ny = static_cast<std::int64_t>(tile / width) + dy;

                if (nx < 0 || ny < 0 || nx >= static_cast<std::int64_t>(width) || ny >= static_cast<std::int64_t>(height)) {
                    continue;
                }

                const int step = opening_cost(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
                const std::size_t next = static_cast<std::size_t>(ny) * width + static_cast<std::size_t>(nx);

                if (step != closed && so_far + step < cost[next]) {
                    if (cost[next] == unknown) {
                        touched.push_back(next);
                    }

                    cost[next] = so_far + step;
                    from[next] = tile;
                    queue.emplace(cost[next], next);
                }
            }
        }

        if (!goal) {
            ++result.regions_unreachable;
            continue;
        }

        // Open the way, and count what it reaches as reached
        for (std::size_t tile = *goal; from[tile] != tile; tile = from[tile]) {
            const std::size_t x = tile % width;
            const std::size_t y = tile / width;
            const std::string_view type = map.get(x, y).getType();

            for (const SOpening& opening : openings) {
                if (type == opening.type) {
                    ++result.opened[std::string(type)];
                    map.SetCell(x, y, opening.becomes);
                    break;
                }
            }

            reached[tile] = true;
        }

        for (const std::size_t tile : members[region]) {
            reached[tile] = true;
        }

        ++result.regions_joined;
    }

    return result;
}

} // namespace RL

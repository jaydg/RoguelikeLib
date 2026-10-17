/**
 * @file connectivity.cppm
 * @brief Map region connectivity analysis, flood-fill segmentation, and minimum-cost opening repairs.
 *
 * Provides algorithms to detect disconnected components across a @ref RL::CMap and carve
 * minimal-cost pathways through obstacles (e.g. chopping trees, bridging streams, or digging walls)
 * to guarantee that all playable areas are interconnected.
 */

module;

export module rl.connectivity;

export import rl.map;
export import rl.matrix;
export import rl.position;
import rl.tile;
import std;

namespace RL
{

/**
 * @brief Returns coordinate delta steps for neighbor traversal.
 *
 * @param neighbors Neighbor connectivity mode (@ref Neighbors::All8 or @ref Neighbors::Cardinal4).
 * @return Span of (dx, dy) coordinate deltas.
 */
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

/**
 * @brief Connected components segmentation of a map into isolated walkable regions.
 *
 * Produced by @ref FindRegions to catalog all disconnected pockets of walkable terrain.
 * Each walkable cell is assigned a zero-based integer region identifier, while impassable
 * obstacles remain marked with `-1`.
 *
 * @see FindRegions
 * @see MakeReachable
 */
struct SRegions {
    /**
     * @brief 2D matrix storing the region ID for each cell.
     *
     * Value is `>= 0` for walkable tiles belonging to a region, or `-1` for impassable
     * obstacles, walls, or unvisited tiles.
     */
    CMatrix<int> region;

    /**
     * @brief Number of walkable tiles contained in each region.
     *
     * Indexed by region identifier: `sizes[id]` contains the tile count for region `id`.
     */
    std::vector<std::size_t> sizes;

    /**
     * @brief Finds the region identifier with the largest surface area.
     *
     * In the event of a tie, the lowest region index among the largest is returned.
     *
     * @return Region ID of the largest component, or `-1` if the map contains no walkable tiles.
     */
    [[nodiscard]] int Largest() const
    {
        if (sizes.empty()) {
            return -1;
        }

        return static_cast<int>(std::ranges::max_element(sizes) - sizes.begin());
    }
};

/**
 * @brief Flood-fills a map to identify all disconnected walkable regions.
 *
 * Traverses all passable cells (@ref CTile::isPassable) using iterative depth-first flood-fill,
 * assigning a unique region ID to each isolated cluster.
 *
 * @param map Map to inspect.
 * @param neighbors Adjacency mode: @ref Neighbors::All8 treats diagonal steps as connecting tiles;
 *                  @ref Neighbors::Cardinal4 requires orthogonal adjacency.
 * @return @ref SRegions structure detailing tile-to-region mappings and region size counts.
 *
 * @code{.cpp}
 * RL::CMap map({80, 50});
 * // ... populate map with rooms, corridors, and walls ...
 *
 * RL::SRegions regions = RL::FindRegions(map, RL::Neighbors::All8);
 * int main_id = regions.Largest();
 * std::cout << "Discovered " << regions.sizes.size() << " regions; main area has "
 *           << (main_id >= 0 ? regions.sizes[main_id] : 0) << " tiles.\n";
 * @endcode
 *
 * @see SRegions
 * @see MakeReachable
 */
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

/**
 * @brief Specification for opening or converting an impassable obstacle to connect regions.
 *
 * Used by @ref MakeReachable to define which obstacle tile archetypes can be excavated,
 * chopped down, bridged, or cleared, along with their relative economic or pathing costs.
 *
 * @code{.cpp}
 * // Cut through trees easily, build bridges over water at higher cost:
 * constexpr std::array<RL::SOpening, 2> rules = {{
 *     {"tree", 2, "grass"},
 *     {"water", 5, "bridge"}
 * }};
 * @endcode
 *
 * @see MakeReachable
 * @see SReachability
 */
struct SOpening {
    std::string_view type;    ///< Archetype name of the impassable tile to open (e.g., `"tree"`, `"wall"`, `"water"`).
    int cost = 1;             ///< Excavation or opening cost (must be >= 1). Lower-cost alternatives are chosen first.
    std::string_view becomes; ///< Archetype name of the replacement walkable tile (e.g., `"grass"`, `"floor"`, `"bridge"`).
};

/**
 * @brief Summary report returned after executing connectivity repairs.
 *
 * Records how many disjoint components were connected, which obstacles were opened up,
 * and if any regions remained isolated.
 *
 * @see MakeReachable
 */
struct SReachability {
    std::size_t regions_joined = 0;                         ///< Number of disconnected regions successfully merged into the primary region.
    std::map<std::string, std::size_t, std::less<>> opened; ///< Count of modified tiles keyed by original tile archetype.
    std::size_t regions_unreachable = 0;                    ///< Number of isolated regions that could not be reached via permitted openings.
};

/**
 * @brief Connects all isolated map regions to the largest component via minimum-cost pathways.
 *
 * Discovers all disjoint walkable regions via @ref FindRegions and connects each isolated
 * pocket to the primary (largest) region by carving through intervening obstacles. Only obstacle
 * types explicitly listed in @p openings will ever be modified; any unlisted impassable terrain
 * remains untouched.
 *
 * Algorithm behavior:
 * - **Deterministic Dijkstra:** Uses a multi-source shortest-path search prioritized by cumulative
 *   opening costs. Identical maps and rules always yield identical results without PRNG jitter.
 * - **Size-Ordered Joining:** Attempts to join larger regions first. Connecting a large region
 *   often creates corridors that serendipitously bridge smaller neighboring regions along the way.
 * - **Minimal Invasiveness:** Only modifies the exact tiles along the optimal path, updating them
 *   in-place via @ref CMap::SetCell.
 * - **Targeted State Resets:** Tracks touched cells to avoid clearing entire grid matrices between
 *   iterations, maintaining high performance even with dozens of disconnected pockets.
 *
 * @param map Map to analyze and modify in-place.
 * @param openings Span of @ref SOpening rules defining permissible tile transformations and costs.
 * @param neighbors Adjacency mode (@ref Neighbors::All8 or @ref Neighbors::Cardinal4).
 * @return @ref SReachability summary detailing modified tiles and connection metrics.
 *
 * @throws std::invalid_argument If any @ref SOpening has a cost < 1.
 *
 * @code{.cpp}
 * constexpr std::array<RL::SOpening, 3> forest_openings = {{
 *     {"water", 2, "bridge"},
 *     {"tree", 3, "grass"},
 *     {"rock", 4, "grass"}
 * }};
 *
 * RL::SReachability report = RL::MakeReachable(map, forest_openings);
 * std::cout << "Merged " << report.regions_joined << " regions.\n";
 * for (const auto& [type, count] : report.opened) {
 *     std::cout << "  Converted " << count << " tiles of type '" << type << "'\n";
 * }
 * @endcode
 *
 * @see SOpening
 * @see SReachability
 * @see FindRegions
 */
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

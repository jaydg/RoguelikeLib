/**
 * @file riverland.cppm
 * @brief Procedural wilderness map generator featuring meandering rivers, tributaries, clearings, and connecting roads.
 */

module;

export module rl.mapgenerators.riverland;

export import rl.connectivity;
export import rl.locations;
export import rl.mapgenerators.biomes;
import rl.map;
import rl.maputils;
import rl.matrix;
import rl.tile;
import rl.position;
import rl.randomness;
import rl.distance;
import std;

export namespace RL
{

// =============================================================================
// Tools for natural clearings
// =============================================================================

/**
 * @brief Generates an organic, irregular clearing and records its constituent tiles.
 * @param map Destination map.
 * @param center Center coordinate of the clearing.
 * @param radius Base radius of the clearing.
 * @param[out] all_clearing_tiles Vector appended with every cell set to the clearing type.
 * @param clearing_type Tile type name to assign (defaults to `"grass"`).
 */
void GenerateAndTrackClearing(
    CMap& map,
    const Position& center,
    std::size_t radius,
    std::vector<Position>& all_clearing_tiles,
    std::string_view clearing_type = "grass"
)
{
    const std::size_t max_radius = radius * 2;
    const float noise_scale = 0.3f; // Scaling for "randomness" of the shape

    for (int x = std::max(0, static_cast<int>(center.x) - static_cast<int>(max_radius));
            x <= std::min(static_cast<int>(map.getWidth()) - 1, static_cast<int>(center.x) + static_cast<int>(max_radius));
            ++x) {
        for (int y = std::max(0, static_cast<int>(center.y) - static_cast<int>(max_radius));
                y <= std::min(static_cast<int>(map.getHeight()) - 1, static_cast<int>(center.y) + static_cast<int>(max_radius));
                ++y) {
            // Calculate the distance to the center
            float dist = Distance(x, y, center.x, center.y);

            if (dist > max_radius) {
                continue;
            }

            // Add random "noise" to create irregular shapes
            float noise = (RandomFloat() * 2.0f - 1.0f) * noise_scale * max_radius;
            float effective_dist = dist + noise;

            // Place the tile if within radius and not on water
            if (effective_dist <= radius && map.get(x, y).getType() != "water") {
                map.SetCell(x, y, clearing_type);
                all_clearing_tiles.emplace_back(x, y);
            }
        }
    }
}

// =============================================================================
// Helper functions for rivers and streams
// =============================================================================

namespace riverland_detail
{

// A seed for the noise of one river, drawn from the library's generator so
// that a seeded run makes the same rivers.
inline std::uint64_t NoiseSeed()
{
    return (static_cast<std::uint64_t>(Random(1u << 30)) << 32) ^ Random(1u << 30);
}

// Appends a cell to a path. A diagonal step gets a corner cell first, so the
// path stays 4-connected and the water drawn along it cannot be crossed
// diagonally.
inline void AppendFourConnected(std::vector<Position>& path, const Position& next)
{
    if (!path.empty()) {
        const Position last = path.back();

        if (last.x == next.x && last.y == next.y) {
            return;
        }

        if (last.x != next.x && last.y != next.y) {
            path.emplace_back(next.x, last.y);
        }
    }

    path.push_back(next);
}

} // namespace riverland_detail

/**
 * @brief Generates a meandering river course from @p start to @p end.
 *
 * The river is walked in half-cell steps with a heading that turns smoothly.
 * The heading aims at the end, offset by one-dimensional noise along the
 * distance travelled: neighbouring steps wander alike, so the river swings
 * in long bends instead of jittering about a straight line. Near the end the
 * wander fades and the river turns in to meet it.
 *
 * @param map Map supplying the bounds; it is not changed.
 * @param start First cell of the river; must be on the map.
 * @param end Last cell of the river; clamped onto the map.
 * @return The 4-connected cells of the river from @p start to @p end, or
 *         nothing if @p start is not on the map.
 */
std::vector<Position> GenerateRiverPath(
    CMap& map,
    const Position& start,
    const Position& end
)
{
    std::vector<Position> path;

    if (!map.inside(start.x, start.y)) {
        return path;
    }

    constexpr double step = 0.5;        // Sub-cell steps keep the bends smooth
    constexpr double wavelength = 10.0; // Cells per noise feature: about half a meander
    constexpr double max_wander = 1.3;  // Largest departure from the bearing to the end (radians)
    constexpr double max_turn = 0.12;   // Largest change of heading per step (radians)
    constexpr double settle = 12.0;     // Closer than this, the river heads for its end

    const double max_x = static_cast<double>(map.getWidth() - 1);
    const double max_y = static_cast<double>(map.getHeight() - 1);
    const Position target(std::min(end.x, map.getWidth() - 1), std::min(end.y, map.getHeight() - 1));
    const double ex = static_cast<double>(target.x);
    const double ey = static_cast<double>(target.y);
    const std::uint64_t seed = riverland_detail::NoiseSeed();

    double px = static_cast<double>(start.x);
    double py = static_cast<double>(start.y);
    double heading = std::atan2(ey - py, ex - px);

    // Enough for a river winding to twice its straight length, and a cap
    // in any case
    const std::size_t max_steps = static_cast<std::size_t>(std::hypot(ex - px, ey - py) / step * 2.0) + 16;

    path.push_back(start);

    for (std::size_t i = 0; i < max_steps; ++i) {
        const double remaining = std::hypot(ex - px, ey - py);

        if (remaining < 1.0) {
            break;
        }

        // The bearing is recomputed every step, so the river cannot wander
        // past its end and keep going
        const double bearing = std::atan2(ey - py, ex - px);
        const double noise = (Fbm(static_cast<double>(i) * step / wavelength, 0.5, seed, 3) - 0.5) * 4.0;
        const double closeness = std::min(1.0, remaining / settle);
        const double wander = std::clamp(noise, -1.0, 1.0) * max_wander * closeness;

        // Near the end, allow sharper turns so the river does not circle it
        const double turn_limit = max_turn / closeness;
        const double turn = std::remainder(bearing + wander - heading, 2.0 * std::numbers::pi);
        heading += std::clamp(turn, -turn_limit, turn_limit);

        px = std::clamp(px + std::cos(heading) * step, 0.0, max_x);
        py = std::clamp(py + std::sin(heading) * step, 0.0, max_y);

        riverland_detail::AppendFourConnected(path, Position(std::lround(px), std::lround(py)));
    }

    // Close the last gap: a cell or two normally, more only if the cap was hit
    for (const Position& p : path.back().BuildBresenhamLine(target)) {
        riverland_detail::AppendFourConnected(path, p);
    }

    return path;
}

/**
 * @brief Draws water along a river course, with a width that changes.
 *
 * The radius grows evenly from @p start_radius at the first cell to
 * @p end_radius at the last, wavers along the way with noise, and swells
 * into a pool at @p pools random places. A radius of 1 or more draws a
 * disc; below 1 a single cell is drawn, widened to two cells with a chance
 * growing as the radius approaches 1, which suits thin streams.
 *
 * @param map Map to draw on.
 * @param path River course, as from @ref GenerateRiverPath.
 * @param start_radius Radius at the first cell of @p path.
 * @param end_radius Radius at the last cell of @p path.
 * @param pools Number of pools; rivers shorter than 20 cells get none.
 * @param[out] painted Appended with every cell drawn.
 * @param river_type Tile type to draw (defaults to `"water"`).
 */
void PaintRiver(
    CMap& map,
    const std::vector<Position>& path,
    double start_radius,
    double end_radius,
    std::size_t pools,
    std::vector<Position>& painted,
    std::string_view river_type = "water"
)
{
    if (path.empty()) {
        return;
    }

    struct SPool {
        double at;    // Index along the path
        double swell; // How much the radius grows at its middle
        double reach; // How far along the path it extends
    };

    std::vector<SPool> pool_list;

    for (std::size_t i = 0; i < pools && path.size() > 20; ++i) {
        pool_list.push_back({
            static_cast<double>(RandomBetween(10, path.size() - 10)),
            static_cast<double>(RandomFloat(1.5f, 3.0f)),
            static_cast<double>(RandomFloat(3.0f, 6.0f))
        });
    }

    const std::uint64_t seed = riverland_detail::NoiseSeed();

    const auto paint = [&](long x, long y) {
        if (x >= 0 && y >= 0 && map.inside(static_cast<std::size_t>(x), static_cast<std::size_t>(y))) {
            map.SetCell(static_cast<std::size_t>(x), static_cast<std::size_t>(y), river_type);
            painted.emplace_back(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        }
    };

    for (std::size_t i = 0; i < path.size(); ++i) {
        const double t = path.size() > 1 ? static_cast<double>(i) / static_cast<double>(path.size() - 1) : 0.0;
        double radius = start_radius + (end_radius - start_radius) * t;

        // Waver by up to about a quarter of the radius
        radius *= 1.0 + (Fbm(static_cast<double>(i) / 12.0, 3.5, seed, 2) - 0.5);

        for (const SPool& pool : pool_list) {
            const double d = (static_cast<double>(i) - pool.at) / pool.reach;
            radius += pool.swell * std::exp(-d * d);
        }

        const long cx = static_cast<long>(path[i].x);
        const long cy = static_cast<long>(path[i].y);

        if (radius < 1.0) {
            paint(cx, cy);

            if (RandomFloat() < (radius - 0.5) * 2.0) {
                paint(cx + 1, cy);
                paint(cx, cy + 1);
            }

            continue;
        }

        const long reach = static_cast<long>(std::ceil(radius));

        for (long dy = -reach; dy <= reach; ++dy) {
            for (long dx = -reach; dx <= reach; ++dx) {
                if (static_cast<double>(dx * dx + dy * dy) <= radius * radius) {
                    paint(cx + dx, cy + dy);
                }
            }
        }
    }
}

/**
 * @brief Generates a random walk river with recursive branches.
 * @param map Map to draw on.
 * @param start Starting position (randomized if out of bounds).
 * @param length Total step length of the main river trunk.
 * @param max_branches Maximum number of tributary branches to sprout.
 * @param river_type Tile type to paint (defaults to `"water"`).
 */
void GenerateBranchedRiver(
    CMap& map,
    const Position& start,
    std::size_t length,
    std::size_t max_branches = 2,
    std::string_view river_type = "water"
)
{
    std::vector<Position> main_path;
    Position current = start;

    // Start anywhere when the start position is not on the map
    if (!map.inside(current.x, current.y)) {
        current = Position(Random(map.getWidth()), Random(map.getHeight()));
    }

    main_path.push_back(current);

    // Generate main river
    for (std::size_t i = 0; i < length; ++i) {
        // Random direction: prefer straight ahead, but with curves
        int dx = 0, dy = 0;

        if (RandomFloat() < 0.7f) {
            // prefer the current direction for natural curves
            if (main_path.size() > 1) {
                dx = static_cast<int>(main_path.back().x) - static_cast<int>(main_path[main_path.size() - 2].x);
                dy = static_cast<int>(main_path.back().y) - static_cast<int>(main_path[main_path.size() - 2].y);

                if (RandomFloat() < 0.5f) {
                    // slight deviation: turn by 45 degrees either way
                    if (CoinToss()) {
                        std::tie(dx, dy) = std::pair{Sign(dx - dy), Sign(dx + dy)};
                    } else {
                        std::tie(dx, dy) = std::pair{Sign(dx + dy), Sign(dy - dx)};
                    }
                }
            } else {
                // starting direction
                dx = Random(3) - 1;
                dy = Random(3) - 1;
            }
        } else {
            // a random new direction
            dx = Random(3) - 1;
            dy = Random(3) - 1;
        }

        // Limit the direction to 1 step per iteration
        if (dx != 0 || dy != 0) {
            dx = std::clamp(dx, -1, 1);
            dy = std::clamp(dy, -1, 1);
        }

        Position next(current.x + dx, current.y + dy);

        if (map.inside(next.x, next.y)) {
            current = next;
            main_path.push_back(current);
        } else {
            // If the path is blocked, try another direction
            for (int attempt = 0; attempt < 5; ++attempt) {
                dx = Random(3) - 1;
                dy = Random(3) - 1;
                next = Position(current.x + dx, current.y + dy);

                if (map.inside(next.x, next.y)) {
                    current = next;
                    main_path.push_back(current);
                    break;
                }
            }
        }
    }

    // Widen the river
    for (const auto& pos : main_path) {
        if (map.inside(pos.x, pos.y)) {
            map.SetCell(pos.x, pos.y, river_type);

            if (pos.x > 0) {
                map.SetCell(pos.x - 1, pos.y, river_type);
            }

            if (pos.y > 0) {
                map.SetCell(pos.x, pos.y - 1, river_type);
            }
        }
    }

    // Generate branches
    for (std::size_t b = 0; b < max_branches; ++b) {
        if (main_path.size() < 5) {
            continue;
        }

        std::size_t branch_start_idx = RandomBetween(1, main_path.size() - 2);
        Position branch_start = main_path[branch_start_idx];
        // At least 5, also when half the river is shorter than that
        std::size_t branch_length = RandomBetween(5, std::max(length / 2, std::size_t{6}));
        GenerateBranchedRiver(map, branch_start, branch_length, 0, river_type);
    }
}

// =============================================================================
// Helper functions for paths and bridges
// =============================================================================

/**
 * @brief Generates a zigzag path between two points, building bridges over water.
 * @param map Map to draw on.
 * @param start Starting position.
 * @param end Destination position.
 * @param path_type Tile type for terrestrial road cells (defaults to `"corridor"`).
 * @param bridge_type Tile type for aquatic bridge crossings (defaults to `"bridge"`).
 * @return True upon path completion.
 */
bool GeneratePathWithBridges(
    CMap& map,
    const Position& start,
    const Position& end,
    std::string_view path_type = "corridor",
    std::string_view bridge_type = "bridge"
)
{
    std::vector<Position> path;
    BuildSigsagPath(path, start, end, 50, 30);

    for (const auto& pos : path) {
        if (!map.inside(pos.x, pos.y)) {
            continue;
        }

        // Check if the path hits water and build bridges
        if (map.get(pos.x, pos.y).getType() == "water") {
            map.SetCell(pos.x, pos.y, bridge_type);
        } else {
            map.SetCell(pos.x, pos.y, path_type);
        }
    }

    return true;
}

/**
 * @brief Distributes rocks randomly across dry ground on the map.
 * @param map Map to populate.
 * @param rock_density Probability [0.0, 1.0] of placing a rock on any eligible open land cell.
 */
void GenerateRocks(CMap& map, float rock_density = 0.05f)
{
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            std::string_view current_type = map.get(x, y).getType();

            // Don't place rocks on water, bridges, or paths
            if (current_type != "water" && current_type != "bridge" &&
                    current_type != "corridor" && RandomFloat() < rock_density) {
                map.SetCell(x, y, "rock");
            }
        }
    }
}

// =============================================================================
// Main function: generate forest
// =============================================================================

// What may be opened up to make a forest reachable everywhere: streams and
// rivers are bridged, trees and rocks give way to grass. Trees are cleared
// rather than rocks, and both rather than a long bridge built. See
// MakeReachable() in rl.connectivity.
constexpr std::array<SOpening, 3> ForestOpenings = {{
        {"water", 2, "bridge"},
        {"tree", 3, "grass"},
        {"rock", 4, "grass"}
    }
};

/**
 * @brief Generates an organic wilderness landscape featuring meandering rivers, tributaries, clearings, and connecting roads.
 *
 * @param map Map to populate.
 * @param biome Terrain pattern defining trees, foliage, and vegetation parameters (defaults to @ref SForestBiome).
 * @param num_clearings Target number of open forest clearings (defaults to 5).
 * @param num_paths Number of interconnected road paths linking clearings (defaults to 8).
 * @param num_rivers Number of main meandering rivers traversing the map (defaults to 1).
 * @param num_streams Number of tributary streams feeding into the rivers (defaults to 8).
 * @return Vector of @ref SLocation markers describing the generated clearings.
 */
std::vector<SLocation> CreateRiverland(
    CMap& map,
    const TerrainBiome& biome = SForestBiome{},
    std::size_t num_clearings = 5,
    std::size_t num_paths = 8,
    std::size_t num_rivers = 1,
    std::size_t num_streams = 8
)
{
    // 1. Fill map with grass
    map.Clear("grass");

    // 2. Generate rivers and streams (cut through vegetation)
    std::vector<Position> all_river_positions;

    for (std::size_t i = 0; i < num_rivers; ++i) {
        Position start, end;
        std::size_t entry_side, exit_side;
        int attempts = 0;
        const int max_attempts = 20;

        // Compute minimum required distance: 50% of the longest side
        std::size_t longest_side = std::max(map.getWidth(), map.getHeight());
        float min_distance = longest_side * 0.5f;

        do {
            entry_side = Random(4);
            exit_side = Random(4);

            // Generate start position on entry side
            switch (entry_side) {
            case 0: // Top
                start = Position(Random(map.getWidth()), 0);
                break;

            case 1: // Right
                start = Position(map.getWidth() - 1, Random(map.getHeight()));
                break;

            case 2: // Bottom
                start = Position(Random(map.getWidth()), map.getHeight() - 1);
                break;

            case 3: // Left
                start = Position(0, Random(map.getHeight()));
                break;
            }

            // Generate end position on exit side
            switch (exit_side) {
            case 0: // Top
                end = Position(Random(map.getWidth()), 0);
                break;

            case 1: // Right
                end = Position(map.getWidth() - 1, Random(map.getHeight()));
                break;

            case 2: // Bottom
                end = Position(Random(map.getWidth()), map.getHeight() - 1);
                break;

            case 3: // Left
                end = Position(0, Random(map.getHeight()));
                break;
            }

            attempts++;
        } while ((entry_side == exit_side ||
                  (start.x == end.x && start.y == end.y) ||
                  Distance(start.x, start.y, end.x, end.y) < min_distance) &&
                 attempts < max_attempts);

        // Generate the river course and draw it 3 cells wide at its start,
        // widening to about 4 towards its end, with a pool or two
        const std::vector<Position> river_path = GenerateRiverPath(map, start, end);
        PaintRiver(map, river_path, 1.0, 1.6, 1 + Random(2), all_river_positions);
    }

    // 3. Generate streams: tributaries rising in the woods a little way off
    //    the rivers and flowing into the nearest one, thin at the source and
    //    widening towards the mouth
    for (std::size_t i = 0; i < num_streams; ++i) {
        if (all_river_positions.empty()) {
            // No rivers to flow into: a short standalone stream
            Position start(Random(map.getWidth()), Random(map.getHeight()));
            std::size_t length = RandomBetween(5, 15);
            GenerateBranchedRiver(map, start, length, 0, "water");
            continue;
        }

        // Find a source between 8 and 25 cells from the nearest river cell
        constexpr std::size_t min_length = 8;
        constexpr std::size_t max_length = 25;
        bool found = false;
        Position source, mouth;

        for (int attempt = 0; attempt < 30 && !found; ++attempt) {
            const Position candidate(Random(map.getWidth()), Random(map.getHeight()));
            std::size_t nearest = std::numeric_limits<std::size_t>::max();

            for (const auto& water : all_river_positions) {
                const std::size_t distance = Distance(candidate.x, candidate.y, water.x, water.y);

                if (distance < nearest) {
                    nearest = distance;
                    mouth = water;
                }
            }

            if (nearest >= min_length && nearest <= max_length) {
                source = candidate;
                found = true;
            }
        }

        if (!found) {
            continue;
        }

        // The stream ends where it first reaches water, which may be
        // another stream on the way
        std::vector<Position> stream_path = GenerateRiverPath(map, source, mouth);

        for (std::size_t s = 0; s < stream_path.size(); ++s) {
            if (map.get(stream_path[s].x, stream_path[s].y).getType() == "water") {
                stream_path.resize(s + 1);
                break;
            }
        }

        PaintRiver(map, stream_path, 0.5, 0.95, 0, all_river_positions);
    }

    // 4. Generate larger organic clearings (after rivers and streams, before paths)
    // Clearings are 12 +/- 2 tiles wide for nice open areas in the forest
    std::vector<Position> clearing_centers;
    std::vector<std::size_t> clearing_radii;
    clearing_centers.reserve(num_clearings * 2);
    clearing_radii.reserve(num_clearings * 2);
    const std::size_t min_clearing_distance = 20; // Increased for larger clearings
    const float min_clearing_percentage = 0.15f;

    std::vector<Position> all_clearing_tiles;

    // Picks a random center at least min_distance away from every existing
    // clearing. When none is found, the candidate farthest from the others is
    // returned in `best`, and the result is false.
    const auto find_clearing_center = [&](std::size_t min_distance, std::size_t max_attempts, Position & best) {
        std::size_t best_distance = 0;

        for (std::size_t attempt = 0; attempt < max_attempts; ++attempt) {
            const Position candidate(Random(map.getWidth()), Random(map.getHeight()));
            std::size_t nearest = std::numeric_limits<std::size_t>::max();

            for (const auto& existing : clearing_centers) {
                nearest = std::min(nearest, Distance(candidate.x, candidate.y, existing.x, existing.y));
            }

            if (attempt == 0 || nearest > best_distance) {
                best = candidate;
                best_distance = nearest;
            }

            if (nearest >= min_distance) {
                return true;
            }
        }

        return false;
    };

    // Overlapping and regrown clearings track the same tile more than once
    const auto count_clearing_tiles = [&all_clearing_tiles]() {
        std::ranges::sort(all_clearing_tiles, [](const Position & a, const Position & b) {
            return a.y < b.y || (a.y == b.y && a.x < b.x);
        });
        all_clearing_tiles.erase(std::unique(all_clearing_tiles.begin(), all_clearing_tiles.end()), all_clearing_tiles.end());

        return all_clearing_tiles.size();
    };

    for (std::size_t i = 0; i < num_clearings; ++i) {
        Position center;
        find_clearing_center(min_clearing_distance, 100, center);

        std::size_t radius = RandomBetween(10, 14); // Larger clearings: 12 +/- 2
        GenerateAndTrackClearing(map, center, radius, all_clearing_tiles, "grass");

        clearing_centers.push_back(center);
        clearing_radii.push_back(radius);
    }

    std::size_t total_tiles = map.getWidth() * map.getHeight();
    std::size_t target_clearing_tiles = static_cast<std::size_t>(total_tiles * min_clearing_percentage);
    std::size_t current_clearing_tiles = count_clearing_tiles();

    while (current_clearing_tiles < target_clearing_tiles) {
        bool added_clearing = false;

        if (clearing_centers.size() < num_clearings * 2) {
            Position center;

            if (find_clearing_center(min_clearing_distance / 2, 50, center)) {
                std::size_t radius = RandomBetween(4, 10);
                GenerateAndTrackClearing(map, center, radius, all_clearing_tiles, "grass");

                clearing_centers.push_back(center);
                clearing_radii.push_back(radius);
                added_clearing = true;
            }
        }

        if (!added_clearing && !clearing_centers.empty()) {
            std::size_t grow_index = Random(clearing_centers.size());
            std::size_t new_radius = std::min(clearing_radii[grow_index] + 2, static_cast<std::size_t>(12));

            if (new_radius > clearing_radii[grow_index]) {
                GenerateAndTrackClearing(map, clearing_centers[grow_index], new_radius, all_clearing_tiles, "grass");
                clearing_radii[grow_index] = new_radius;
                added_clearing = true;
            }
        }

        if (!added_clearing) {
            break;
        }

        current_clearing_tiles = count_clearing_tiles();
    }

    CMatrix<bool> clearing_access(map.getSize(), false);

    // 5. Generate ways connecting clearings (with collision avoidance and bridges)
    if (!clearing_centers.empty()) {
        CMatrix<bool> path_tiles(map.getSize(), false);
        std::vector<std::vector<Position>> entrances(clearing_centers.size());
        const auto index = [&](Position p) { return p.y * map.getWidth() + p.x; };

        // Reuse an entrance for destinations in the same general direction.
        // Opposite destinations can still have separate clearing entrances.
        const auto entrance = [&](std::size_t clearing, Position destination) {
            const Position center = clearing_centers[clearing];
            const double dx = static_cast<double>(destination.x) - center.x;
            const double dy = static_cast<double>(destination.y) - center.y;
            const double length = std::hypot(dx, dy);
            for (Position existing : entrances[clearing]) {
                const double ex = static_cast<double>(existing.x) - center.x;
                const double ey = static_cast<double>(existing.y) - center.y;
                if (ex * dx + ey * dy >= 0.5 * std::hypot(ex, ey) * length) {
                    return existing;
                }
            }
            // Stay just inside the irregular edge, rather than running a
            // road through the middle of the open clearing.
            const double distance = std::min(length * 0.4, clearing_radii[clearing] * 0.8);
            Position result(
                std::clamp<int>(std::lround(center.x + (length > 0 ? dx / length * distance : 0)), 0, map.getWidth() - 1),
                std::clamp<int>(std::lround(center.y + (length > 0 ? dy / length * distance : 0)), 0, map.getHeight() - 1));
            entrances[clearing].push_back(result);
            // Keep access through the clearing walkable without painting a
            // road across its centre. Water still needs a bridge.
            const int steps = std::max(std::abs(static_cast<int>(result.x) - static_cast<int>(center.x)),
                                       std::abs(static_cast<int>(result.y) - static_cast<int>(center.y)));
            for (int step = 0; step <= steps; ++step) {
                const double t = steps > 0 ? static_cast<double>(step) / steps : 0;
                const Position p(std::lround(center.x + (static_cast<double>(result.x) - center.x) * t),
                                 std::lround(center.y + (static_cast<double>(result.y) - center.y) * t));
                clearing_access.set(p.x, p.y, true);
                const auto type = map.get(p).getType();
                if (type == "tree" || type == "plant" || type == "rock") map.SetCell(p.x, p.y, "grass");
                else if (type == "water") map.SetCell(p.x, p.y, "bridge");
            }
            return result;
        };

        CMatrix<bool> clearing_core(map.getSize(), false);
        for (std::size_t y = 0; y < map.getHeight(); ++y) {
            for (std::size_t x = 0; x < map.getWidth(); ++x) {
                for (std::size_t i = 0; i < clearing_centers.size(); ++i) {
                    if (Distance(x, y, clearing_centers[i].x, clearing_centers[i].y) < clearing_radii[i] * 0.5)
                        clearing_core.set(x, y, true);
                }
            }
        }

        const auto connect = [&](Position start, Position end) {
            // A winding guide gives new roads a natural shape. Routing can
            // depart from it to reuse a trunk or join an existing road.
            std::vector<Position> guide;
            BuildWindingPath(guide, map, start, end, 8);
            CMatrix<bool> near_guide(map.getSize(), false);
            CMatrix<bool> near_road(map.getSize(), false);
            const auto mark_near = [&](CMatrix<bool>& mask, Position p, int radius) {
                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        const Position at(p.x + dx, p.y + dy);
                        if (map.inside(at)) mask.set(at.x, at.y, true);
                    }
                }
            };
            for (Position p : guide) mark_near(near_guide, p, 2);
            for (std::size_t y = 0; y < map.getHeight(); ++y) {
                for (std::size_t x = 0; x < map.getWidth(); ++x) {
                    if (path_tiles.get(x, y)) mark_near(near_road, Position(x, y), 3);
                }
            }
            const auto count = map.getWidth() * map.getHeight();
            std::vector<int> costs(count, std::numeric_limits<int>::max());
            std::vector<std::size_t> parent(count, count);
            using Entry = std::pair<int, std::size_t>;
            std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
            costs[index(start)] = 0;
            pending.emplace(0, index(start));
            while (!pending.empty()) {
                const auto [cost, cell] = pending.top(); pending.pop();
                if (cost != costs[cell]) continue;
                if (cell == index(end)) break;
                const Position here(cell % map.getWidth(), cell / map.getWidth());
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        const Position next(here.x + dx, here.y + dy);
                        if (!map.inside(next)) continue;
                        // Actual road tiles are cheap; running beside a road
                        // is expensive, so routes share tiles before forking.
                        int step = path_tiles.get(next.x, next.y) ? 2 :
                            10 + (near_guide.get(next.x, next.y) ? 0 : 8) +
                            (near_road.get(next.x, next.y) ? 24 : 0) +
                            (clearing_core.get(next.x, next.y) ? 80 : 0);
                        if (map.get(next).getType() == "water") step += 30;
                        if (dx != 0 && dy != 0) step = step * 14 / 10;
                        const auto target = index(next);
                        if (cost + step < costs[target]) {
                            costs[target] = cost + step;
                            parent[target] = cell;
                            pending.emplace(cost + step, target);
                        }
                    }
                }
            }
            for (auto cell = index(end); cell != count; cell = parent[cell]) {
                const Position p(cell % map.getWidth(), cell / map.getWidth());
                map.SetCell(p.x, p.y, map.get(p).getType() == "water" || map.get(p).getType() == "bridge" ? "bridge" : "corridor");
                path_tiles.set(p.x, p.y, true);
            }
        };

        // First connect every clearing to the closest earlier clearing.
        // Subsequent routes reuse existing entrances and road segments.
        for (std::size_t i = 1; i < clearing_centers.size(); ++i) {
            std::size_t nearest = 0;
            for (std::size_t j = 1; j < i; ++j) {
                if (Distance(clearing_centers[i].x, clearing_centers[i].y, clearing_centers[j].x, clearing_centers[j].y) <
                    Distance(clearing_centers[i].x, clearing_centers[i].y, clearing_centers[nearest].x, clearing_centers[nearest].y)) nearest = j;
            }
            const Position start = entrance(i, clearing_centers[nearest]);
            const Position end = entrance(nearest, clearing_centers[i]);
            connect(start, end);
        }
        for (std::size_t extra = 0; extra < num_paths && clearing_centers.size() > 1; ++extra) {
            const std::size_t a = Random(clearing_centers.size()), b = Random(clearing_centers.size());
            if (a == b) continue;
            const Position start = entrance(a, clearing_centers[b]);
            const Position end = entrance(b, clearing_centers[a]);
            connect(start, end);
        }
    } else {
        // Fallback: generate random paths if no clearings were created
        for (std::size_t i = 0; i < num_paths; ++i) {
            Position start(Random(map.getWidth()), Random(map.getHeight()));
            Position end(Random(map.getWidth()), Random(map.getHeight()));
            GeneratePathWithBridges(map, start, end, "corridor", "bridge");
        }
    }

    // 6. Fill the wilderness background outside clearings, access ways, rivers and roads
    CMatrix<bool> clearing_mask(map.getSize(), false);
    for (const auto& pos : all_clearing_tiles) {
        if (map.inside(pos)) {
            clearing_mask.set(pos.x, pos.y, true);
        }
    }

    CMatrix<bool> mask(map.getSize(), false);
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            const auto type = map.get(x, y).getType();
            if (type == "grass" && !clearing_mask.get(x, y) && !clearing_access.get(x, y)) {
                mask.set(x, y, true);
            }
        }
    }

    FillBiome(map, mask, biome);

    std::vector<SLocation> clearings;
    clearings.reserve(clearing_centers.size());

    for (std::size_t i = 0; i < clearing_centers.size(); ++i) {
        clearings.push_back({ELocationType::Clearing, clearing_centers[i], clearing_radii[i]});
    }

    return clearings;
}

/**
 * @brief Convenience overload of @ref CreateRiverland configuring a basic @ref SForestBiome.
 */
inline std::vector<SLocation> CreateRiverland(
    CMap& map,
    float tree_density,
    float rock_density = 0.05f,
    std::size_t num_clearings = 5,
    std::size_t num_paths = 8,
    std::size_t num_rivers = 1,
    std::size_t num_streams = 8,
    float vegetation_density = 0.6f
)
{
    return CreateRiverland(
        map,
        SForestBiome{
            .tree_density = tree_density,
            .rock_density = rock_density,
            .vegetation_density = vegetation_density,
            .patchiness = 0.0f
        },
        num_clearings,
        num_paths,
        num_rivers,
        num_streams
    );
}

// Backward-compatibility aliases for GenerateForest

/**
 * @brief Backward-compatibility alias for @ref CreateRiverland(CMap&, const TerrainBiome&, std::size_t, std::size_t, std::size_t, std::size_t).
 */
inline std::vector<SLocation> GenerateForest(
    CMap& map,
    const TerrainBiome& biome = SForestBiome{},
    std::size_t num_clearings = 5,
    std::size_t num_paths = 8,
    std::size_t num_rivers = 1,
    std::size_t num_streams = 8
)
{
    return CreateRiverland(map, biome, num_clearings, num_paths, num_rivers, num_streams);
}

/**
 * @brief Backward-compatibility alias for @ref CreateRiverland(CMap&, float, float, std::size_t, std::size_t, std::size_t, std::size_t, float).
 */
inline std::vector<SLocation> GenerateForest(
    CMap& map,
    float tree_density,
    float rock_density = 0.05f,
    std::size_t num_clearings = 5,
    std::size_t num_paths = 8,
    std::size_t num_rivers = 1,
    std::size_t num_streams = 8,
    float vegetation_density = 0.6f
)
{
    return CreateRiverland(
        map,
        tree_density,
        rock_density,
        num_clearings,
        num_paths,
        num_rivers,
        num_streams,
        vegetation_density
    );
}

} // namespace RL

module;

export module rl.mapgenerators.forest;

export import rl.connectivity;
export import rl.locations;
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

// Helper function to generate a clearing and track its tiles
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

// Generate a meandering river path from start to end
std::vector<Position> GenerateRiverPath(
    CMap& map,
    const Position& start,
    const Position& end
)
{
    std::vector<Position> path;
    Position current = start;
    path.push_back(current);

    // Calculate target direction
    int target_dx = (end.x > start.x) ? 1 : ((end.x < start.x) ? -1 : 0);
    int target_dy = (end.y > start.y) ? 1 : ((end.y < start.y) ? -1 : 0);

    // Meander towards the target
    while (current.x != end.x || current.y != end.y) {
        int dx = 0, dy = 0;

        // Prefer moving towards the target
        if (current.x != end.x) {
            dx = target_dx;
        }

        if (current.y != end.y) {
            dy = target_dy;
        }

        // Add some random meandering (30% chance to deviate)
        if (RandomFloat() < 0.3f && path.size() > 1) {
            // Get previous direction
            Position prev = path[path.size() - 2];
            int prev_dx = static_cast<int>(current.x) - static_cast<int>(prev.x);
            int prev_dy = static_cast<int>(current.y) - static_cast<int>(prev.y);

            // Try to continue in the same general direction
            if (RandomFloat() < 0.5f) {
                // Continue previous direction with possible slight turn
                dx = prev_dx;
                dy = prev_dy;
            } else {
                // Random deviation
                dx = Random(3) - 1;
                dy = Random(3) - 1;
            }
        }

        // Ensure we're making progress towards the target
        if (RandomFloat() < 0.7f) {
            // Bias towards target direction
            if (RandomFloat() < 0.5f && current.x != end.x) {
                dx = target_dx;
            } else if (current.y != end.y) {
                dy = target_dy;
            }
        }

        // Limit to single step
        dx = std::clamp(dx, -1, 1);
        dy = std::clamp(dy, -1, 1);

        // Don't allow both to be zero
        if (dx == 0 && dy == 0) {
            dx = target_dx;
            dy = target_dy;
        }

        Position next(current.x + dx, current.y + dy);

        // Check if next position is valid
        bool valid = map.inside(next.x, next.y);

        if (valid) {
            current = next;
            path.push_back(current);
        } else {
            // Try alternative directions
            bool found = false;

            for (int attempt = 0; attempt < 5 && !found; ++attempt) {
                // Prefer directions that make progress towards target
                if (RandomFloat() < 0.5f && current.x != end.x) {
                    dx = target_dx;
                    dy = (RandomFloat() < 0.5f) ? target_dy : 0;
                } else if (current.y != end.y) {
                    dy = target_dy;
                    dx = (RandomFloat() < 0.5f) ? target_dx : 0;
                } else {
                    dx = Random(3) - 1;
                    dy = Random(3) - 1;
                }

                dx = std::clamp(dx, -1, 1);
                dy = std::clamp(dy, -1, 1);

                next = Position(current.x + dx, current.y + dy);
                valid = map.inside(next.x, next.y);

                if (valid) {
                    current = next;
                    path.push_back(current);
                    found = true;
                }
            }

            // If we're stuck, just end the river here
            if (!found) {
                break;
            }
        }
    }

    return path;
}

// Generate a river with branches
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
        float dir = RandomFloat() * 2.0f * 3.1415926535f; // radom angle

        if (dir < 0.7f) {
            // prefer the current direction for natural curves
            if (!main_path.empty() && i > 0) {
                dx = static_cast<int>(main_path.back().x) - static_cast<int>(main_path[main_path.size() - 2].x);
                dy = static_cast<int>(main_path.back().y) - static_cast<int>(main_path[main_path.size() - 2].y);

                if (RandomFloat() < 0.5f) {
                    // slight deviation
                    if (RandomFloat() < 0.5f) {
                        dx = -dy;
                    } else {
                        dy = -dx;
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

// Generates a path with collision avoidance and bridges
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

// Places rocks randomly on the map, avoiding water and paths
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

// @param tree_density       Share of vegetation that is trees, not plants (0.0–1.0, def. 0.7)
// @param rock_density       Probability of rocks (0.0–1.0) (def. 0.05)
// @param num_clearings      Number of clearings (def. 5)
// @param num_paths          Optional extra connection attempts (def. 8);
//                           routes may reuse existing roads entirely
// @param num_rivers         Number of rivers (def. 1)
// @param num_streams        Number of streams (def. 8)
// @param vegetation_density Target vegetation coverage (0.0-1.0, def. 0.6)
//
// @return The clearings, as locations of type Clearing. A clearing is
//         grass everywhere within 0.4 times its radius of the center,
//         save for water, paths and rocks; further out it frays, and at
//         1.6 times its radius it ends. Clearings may overlap. With more
//         than one clearing, roads connect entrances near their edges, sharing
//         trunks where routes overlap; clearing centres remain open.

std::vector<SLocation> GenerateForest(
    CMap& map,
    float tree_density = 0.7f,
    float rock_density = 0.05f,
    std::size_t num_clearings = 5,
    std::size_t num_paths = 8,
    std::size_t num_rivers = 1,
    std::size_t num_streams = 8,
    float vegetation_density = 0.6f
)
{
    // 1. Fill map with grass
    map.Clear("grass");

    // 2. Generate dense vegetation (forest base layer)
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            if (RandomFloat() < vegetation_density) {
                map.SetCell(x, y, RandomFloat() < tree_density ? "tree" : "plant");
            }
        }
    }

    // 3. Generate rivers and streams (cut through vegetation)
    // This way they won't be blocked and will carve through the forest
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

        // Generate the river path
        std::vector<Position> river_path = GenerateRiverPath(map, start, end);

        // Widen the river to 3-4 cells
        // Since rivers come before clearings and rocks, we only need to check map bounds
        for (const auto& pos : river_path) {
            if (!map.inside(pos.x, pos.y)) {
                continue;
            }

            map.SetCell(pos.x, pos.y, "water");
            all_river_positions.push_back(pos);

            // Left
            if (pos.x > 0) {
                map.SetCell(pos.x - 1, pos.y, "water");
                all_river_positions.push_back(Position(pos.x - 1, pos.y));
            }

            // Right
            if (pos.x + 1 < map.getWidth()) {
                map.SetCell(pos.x + 1, pos.y, "water");
                all_river_positions.push_back(Position(pos.x + 1, pos.y));
            }

            // Up
            if (pos.y > 0) {
                map.SetCell(pos.x, pos.y - 1, "water");
                all_river_positions.push_back(Position(pos.x, pos.y - 1));
            }

            // Down
            if (pos.y + 1 < map.getHeight()) {
                map.SetCell(pos.x, pos.y + 1, "water");
                all_river_positions.push_back(Position(pos.x, pos.y + 1));
            }
        }

    }

    // 6. Generate streams that connect to rivers (2 cells wide)
    for (std::size_t i = 0; i < num_streams; ++i) {
        if (all_river_positions.empty()) {
            // No rivers to connect to, skip or generate standalone stream
            Position start(Random(map.getWidth()), Random(map.getHeight()));
            std::size_t length = RandomBetween(5, 15);
            GenerateBranchedRiver(map, start, length, 0, "water");
            continue;
        }

        // Start stream from a random river position
        Position start = all_river_positions[Random(all_river_positions.size())];

        // Choose a direction away from the river center
        // Pick a random direction to flow
        Position end;
        int attempts = 0;
        const int max_attempts = 10;

        do {
            // Generate a point in a random direction from start
            int offset_x = RandomBetween(5, 20);
            int offset_y = RandomBetween(5, 20);

            // Randomly choose which direction to go
            switch (Random(4)) {
            case 0:
                end = Position(start.x + offset_x, start.y + offset_y);
                break;

            case 1:
                end = Position(start.x + offset_x, start.y - offset_y);
                break;

            case 2:
                end = Position(start.x - offset_x, start.y + offset_y);
                break;

            case 3:
                end = Position(start.x - offset_x, start.y - offset_y);
                break;
            }

            attempts++;
        } while (!map.inside(end.x, end.y) && attempts < max_attempts);

        if (!map.inside(end.x, end.y)) {
            continue; // Couldn't find valid end
        }

        // Generate stream path
        std::vector<Position> stream_path = GenerateRiverPath(map, start, end);

        // Make stream 1-2 cells wide
        for (const auto& pos : stream_path) {
            if (map.inside(pos.x, pos.y) && map.get(pos.x, pos.y).getType() != "water") {
                map.SetCell(pos.x, pos.y, "water");
                all_river_positions.push_back(pos);

                // 50% chance to add a second cell for width
                if (RandomFloat() < 0.5f) {
                    // Try to add width in one direction
                    std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                    auto dir = directions[Random(directions.size())];
                    std::size_t wx = pos.x + dir.first;
                    std::size_t wy = pos.y + dir.second;

                    if (map.inside(wx, wy) && map.get(wx, wy).getType() != "water") {
                        map.SetCell(wx, wy, "water");
                        all_river_positions.push_back(Position(wx, wy));
                    }
                }
            }
        }
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

    // 6. Place rocks (after rivers, streams, clearings, and paths)
    GenerateRocks(map, rock_density);
    // Scattered rocks must not close the unpaved access to an entrance.
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            if (clearing_access.get(x, y) && map.get(x, y).getType() == "rock") map.SetCell(x, y, "grass");
        }
    }

    std::vector<SLocation> clearings;
    clearings.reserve(clearing_centers.size());

    for (std::size_t i = 0; i < clearing_centers.size(); ++i) {
        clearings.push_back({ELocationType::Clearing, clearing_centers[i], clearing_radii[i]});
    }

    return clearings;
}

} // namespace RL

module;

export module rl.mapgenerators.forest;

import rl.map;
import rl.maputils;
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

    for (std::size_t x = std::max(0, static_cast<int>(center.x) - static_cast<int>(max_radius));
            x <= std::min(static_cast<int>(map.getWidth()) - 1, static_cast<int>(center.x) + static_cast<int>(max_radius));
            ++x) {
        for (std::size_t y = std::max(0, static_cast<int>(center.y) - static_cast<int>(max_radius));
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

    // Try to find a valid start position
    std::size_t attempts = 0;
    const std::size_t max_attempts = 20;

    while (attempts < max_attempts) {
        current = Position(Random(map.getWidth()), Random(map.getHeight()));
        attempts++;
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
        std::size_t branch_length = RandomBetween(5, length / 2);
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

// @param tree_density       Share of vegetation that is trees, not plants (0.0–1.0, def. 0.7)
// @param rock_density       Probability of rocks (0.0–1.0) (def. 0.05)
// @param num_clearings      Number of clearings (def. 5)
// @param num_paths          Number of paths (def. 8)
// @param num_rivers         Number of rivers (def. 1)
// @param num_streams        Number of streams (def. 8)
// @param vegetation_density Target vegetation coverage (0.0-1.0, def. 0.6)

void GenerateForest(
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

    for (std::size_t i = 0; i < num_clearings; ++i) {
        Position center;
        bool valid_position = false;
        std::size_t attempts = 0;
        const std::size_t max_attempts = 100;

        while (!valid_position && attempts < max_attempts) {
            center = Position(Random(map.getWidth()), Random(map.getHeight()));
            valid_position = true;

            for (const auto& existing : clearing_centers) {
                std::size_t dx = std::abs(static_cast<int>(center.x) - static_cast<int>(existing.x));
                std::size_t dy = std::abs(static_cast<int>(center.y) - static_cast<int>(existing.y));

                if ((dx < min_clearing_distance || dy < min_clearing_distance) || (dx == 0 || dy == 0)) {
                    valid_position = false;
                    break;
                }
            }

            attempts++;
        }

        if (!valid_position) {
            center = Position(Random(map.getWidth()), Random(map.getHeight()));
        }

        std::size_t radius = RandomBetween(10, 14); // Larger clearings: 12 +/- 2
        GenerateAndTrackClearing(map, center, radius, all_clearing_tiles, "grass");

        clearing_centers.push_back(center);
        clearing_radii.push_back(radius);
    }

    std::size_t total_tiles = map.getWidth() * map.getHeight();
    std::size_t target_clearing_tiles = static_cast<std::size_t>(total_tiles * min_clearing_percentage);
    std::size_t current_clearing_tiles = all_clearing_tiles.size();

    while (current_clearing_tiles < target_clearing_tiles) {
        bool added_clearing = false;

        if (clearing_centers.size() < num_clearings * 2) {
            Position center;
            bool valid_position = false;
            std::size_t attempts = 0;
            const std::size_t max_attempts = 50;

            while (!valid_position && attempts < max_attempts) {
                center = Position(Random(map.getWidth()), Random(map.getHeight()));
                valid_position = true;

                for (const auto& existing : clearing_centers) {
                    std::size_t dx = std::abs(static_cast<int>(center.x) - static_cast<int>(existing.x));
                    std::size_t dy = std::abs(static_cast<int>(center.y) - static_cast<int>(existing.y));

                    if ((dx < min_clearing_distance / 2 || dy < min_clearing_distance / 2) || (dx == 0 || dy == 0)) {
                        valid_position = false;
                        break;
                    }
                }

                attempts++;
            }

            if (valid_position) {
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

        current_clearing_tiles = all_clearing_tiles.size();
    }

    // 5. Generate ways connecting clearings (with collision avoidance and bridges)
    if (!clearing_centers.empty()) {
        const std::size_t min_path_distance = 1; // Minimum distance between paths
        std::vector<Position> all_path_positions; // Track all path positions for distance checking

        // Helper function to check if a position is near any existing path
        auto IsNearPath = [&](const Position & pos) -> bool {
            for (const auto& p : all_path_positions)
            {
                std::size_t dx = std::abs(static_cast<int>(pos.x) - static_cast<int>(p.x));
                std::size_t dy = std::abs(static_cast<int>(pos.y) - static_cast<int>(p.y));

                if (dx <= min_path_distance && dy <= min_path_distance) {
                    return true;
                }
            }

            return false;
        };

        // Helper function to check if a position is near any OTHER clearing center
        // (i.e., clearing centers that are NOT the endpoints of this path)
        // Clearings have radius 3-8, so use a buffer to ensure paths go around them
        const std::size_t path_clearing_buffer = 3; // Minimum distance from clearing center for path segments
        auto IsNearOtherClearing = [&](const Position & pos, const Position & path_start, const Position & path_end) -> bool {
            for (const auto& center : clearing_centers)
            {
                // Skip the path endpoints themselves
                if (Distance(pos.x, pos.y, path_start.x, path_start.y) <= 1.0f) {
                    continue;
                }

                if (Distance(pos.x, pos.y, path_end.x, path_end.y) <= 1.0f) {
                    continue;
                }

                // If within buffer distance of any OTHER clearing center, reject
                float dist = Distance(pos.x, pos.y, center.x, center.y);

                if (dist <= path_clearing_buffer) {
                    return true;
                }
            }

            return false;
        };

        // Helper function to check if a position is near any clearing except the two specified endpoints
        // Use different buffers for main paths vs extra paths
        auto IsNearAnyClearing = [&](const Position & pos, const Position & exclude1, const Position & exclude2, bool strict = false) -> bool {
            for (const auto& center : clearing_centers)
            {
                // Skip the two endpoint clearings
                if (Distance(pos.x, pos.y, exclude1.x, exclude1.y) <= 1.0f) {
                    continue;
                }

                if (Distance(pos.x, pos.y, exclude2.x, exclude2.y) <= 1.0f) {
                    continue;
                }

                // If within buffer distance of any OTHER clearing center, reject
                float dist = Distance(pos.x, pos.y, center.x, center.y);
                float buffer = strict ? 5.0f : 1.0f;  // Strict for extra paths, minimal for main paths

                if (dist <= buffer) {
                    return true;
                }
            }

            return false;
        };

        // Build a path network connecting all clearings
        // Use a simple approach: connect each clearing to the next, forming a chain or tree
        std::vector<bool> connected(clearing_centers.size(), false);

        // Always connect the first clearing
        if (!clearing_centers.empty()) {
            connected[0] = true;
        }

        // Connect remaining clearings to the network
        for (std::size_t i = 1; i < clearing_centers.size(); ++i) {
            // Find the closest already-connected clearing
            std::size_t best_j = 0;
            float best_dist = std::numeric_limits<float>::max();

            for (std::size_t j = 0; j < i; ++j) {
                if (connected[j]) {
                    float dist = Distance(
                                     clearing_centers[i].x, clearing_centers[i].y,
                                     clearing_centers[j].x, clearing_centers[j].y
                                 );

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_j = j;
                    }
                }
            }

            // Connect clearing i to clearing best_j
            std::vector<Position> path;
            bool path_valid = false;
            int path_attempts = 0;
            const int max_path_attempts = 3;

            // Try multiple path variations if the first one fails
            while (!path_valid && path_attempts < max_path_attempts) {
                path.clear();
                // Vary the path parameters to get different routes
                unsigned turnpct = 50 + Random(20); // 50-70%
                unsigned diagpct = 30 + Random(20);  // 30-50%
                BuildSigsagPath(path, clearing_centers[i], clearing_centers[best_j], turnpct, diagpct);

                // Check if path crosses any clearing or is too close to existing paths
                path_valid = true;

                for (const auto& pos : path) {
                    // Only check positions that are not the endpoints themselves
                    if (Distance(pos.x, pos.y, clearing_centers[i].x, clearing_centers[i].y) <= 1.0f) {
                        continue;
                    }

                    if (Distance(pos.x, pos.y, clearing_centers[best_j].x, clearing_centers[best_j].y) <= 1.0f) {
                        continue;
                    }

                    // For the main connecting paths, use a lenient clearing buffer to ensure connectivity
                    if (IsNearAnyClearing(pos, clearing_centers[i], clearing_centers[best_j], false) ||
                            (min_path_distance > 0 && IsNearPath(pos))) {
                        path_valid = false;
                        break;
                    }
                }

                path_attempts++;
            }

            if (path_valid) {
                // Add this path
                for (const auto& pos : path) {
                    if (map.inside(pos.x, pos.y)) {
                        // Check if on water for bridge
                        if (map.get(pos.x, pos.y).getType() == "water") {
                            map.SetCell(pos.x, pos.y, "bridge");
                        } else {
                            map.SetCell(pos.x, pos.y, "corridor");
                        }

                        all_path_positions.push_back(pos);
                    }
                }

                connected[i] = true;
            }
        }

        // Add additional paths if we have capacity and want more connections
        // Connect random pairs of already-connected clearings
        for (std::size_t extra = 0; extra < num_paths && clearing_centers.size() > 1; ++extra) {
            std::size_t i1 = Random(clearing_centers.size());
            std::size_t i2 = Random(clearing_centers.size());

            if (i1 == i2) {
                continue;
            }

            std::vector<Position> path;
            bool path_valid = false;
            int path_attempts = 0;
            const int max_path_attempts = 3;

            // Try multiple path variations if the first one fails
            while (!path_valid && path_attempts < max_path_attempts) {
                path.clear();
                // Vary the path parameters to get different routes
                unsigned turnpct = 50 + Random(20); // 50-70%
                unsigned diagpct = 30 + Random(20);  // 30-50%
                BuildSigsagPath(path, clearing_centers[i1], clearing_centers[i2], turnpct, diagpct);

                // Check if path crosses any clearing or is too close to existing paths
                path_valid = true;

                for (const auto& pos : path) {
                    // Only check positions that are not the endpoints themselves
                    if (Distance(pos.x, pos.y, clearing_centers[i1].x, clearing_centers[i1].y) <= 1.0f) {
                        continue;
                    }

                    if (Distance(pos.x, pos.y, clearing_centers[i2].x, clearing_centers[i2].y) <= 1.0f) {
                        continue;
                    }

                    // For extra paths, use strict clearing check
                    if (IsNearAnyClearing(pos, clearing_centers[i1], clearing_centers[i2], true) ||
                            (min_path_distance > 0 && IsNearPath(pos))) {
                        path_valid = false;
                        break;
                    }
                }

                path_attempts++;
            }

            if (path_valid) {
                // Add this path
                for (const auto& pos : path) {
                    if (map.inside(pos.x, pos.y)) {
                        if (map.get(pos.x, pos.y).getType() == "water") {
                            map.SetCell(pos.x, pos.y, "bridge");
                        } else {
                            map.SetCell(pos.x, pos.y, "corridor");
                        }

                        all_path_positions.push_back(pos);
                    }
                }
            }
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
}

} // namespace RL

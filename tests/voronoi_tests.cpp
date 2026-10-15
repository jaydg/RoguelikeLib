import rl.mapgenerators.voronoiwilderness;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{
void CheckBuildings(const RL::CMap& map, const RL::SWildernessInfo& info)
{
    bool bounded = true, faithful = true, clearance = true;
    std::vector<bool> occupied(map.getWidth() * map.getHeight());
    for (const auto& b : info.buildings) {
        const auto width = b.corner2.x - b.corner1.x + 1;
        const auto height = b.corner2.y - b.corner1.y + 1;
        bounded &= width >= 4 && width <= 7 && height >= 4 && height <= 7;
        faithful &= map.get(b.door).getType() == "door_open";
        for (auto y = b.corner1.y; y <= b.corner2.y; ++y) {
            for (auto x = b.corner1.x; x <= b.corner2.x; ++x) {
                const auto cell = y * map.getWidth() + x;
                faithful &= !occupied[cell] && info.difficulty.get(x, y) == 0;
                occupied[cell] = true;
                const bool edge = x == b.corner1.x || x == b.corner2.x || y == b.corner1.y || y == b.corner2.y;
                const auto expected = RL::Position(x, y) == b.door ? "door_open" : edge ? "wall" : "room";
                faithful &= map.get(x, y).getType() == expected;
            }
        }
        for (auto y = b.corner1.y - 1; y <= b.corner2.y + 1; ++y) {
            for (auto x = b.corner1.x - 1; x <= b.corner2.x + 1; ++x) {
                if (x < b.corner1.x || x > b.corner2.x || y < b.corner1.y || y > b.corner2.y) {
                    clearance &= map.get(x, y).isPassable();
                }
            }
        }
    }
    Check(bounded, "each Voronoi house side measures 4–7 tiles including walls");
    Check(faithful, "building metadata matches non-overlapping town footprints and doors");
    Check(clearance, "every house has open clearance around its walls and entrance");
    bool facing = true;
    for (const auto& b : info.buildings) {
        const double cx = (b.corner1.x + b.corner2.x) / 2.0, cy = (b.corner1.y + b.corner2.y) / 2.0;
        const RL::SBuilding* neighbor = nullptr;
        double nearest = std::numeric_limits<double>::max();
        for (const auto& other : info.buildings) {
            if (&other == &b) { continue; }
            const double dx = (other.corner1.x + other.corner2.x) / 2.0 - cx;
            const double dy = (other.corner1.y + other.corner2.y) / 2.0 - cy;
            if (dx * dx + dy * dy < nearest) {
                nearest = dx * dx + dy * dy;
                neighbor = &other;
            }
        }
        if (neighbor) {
            const double dx = (neighbor->corner1.x + neighbor->corner2.x) / 2.0 - cx;
            const double dy = (neighbor->corner1.y + neighbor->corner2.y) / 2.0 - cy;
            const int nx = b.door.x == b.corner1.x ? -1 : b.door.x == b.corner2.x ? 1 : 0;
            const int ny = b.door.y == b.corner1.y ? -1 : b.door.y == b.corner2.y ? 1 : 0;
            facing &= nx * dx + ny * dy > 0;
        }
    }
    Check(facing, "entrances face the nearest neighbouring house");
    if (!info.buildings.empty()) {
        const auto root = info.buildings.front().door;
        std::vector<bool> seen(map.getWidth() * map.getHeight());
        std::vector<RL::Position> queue{root};
        seen[root.y * map.getWidth() + root.x] = true;
        for (std::size_t index = 0; index < queue.size(); ++index) {
            const auto cell = queue[index];
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const auto x = cell.x + dx, y = cell.y + dy;
                    if (x >= map.getWidth() || y >= map.getHeight()) { continue; }
                    const auto type = map.get(x, y).getType();
                    if (!seen[y * map.getWidth() + x] && (type == "corridor" || type == "bridge" || type == "door_open" || type == "field" || type == "plant")) {
                        seen[y * map.getWidth() + x] = true;
                        queue.emplace_back(x, y);
                    }
                }
            }
        }
        Check(std::ranges::all_of(info.buildings, [&](const auto& b) { return seen[b.door.y * map.getWidth() + b.door.x]; }),
              "every entrance joins the shared path network without walking through rooms");
    }

}
}

void TestVoronoiScale()
{
    for (unsigned seed = 0; seed < 6; ++seed) {
        RL::InitRandomness(seed);
        RL::CMap small(RL::Size(79, 50));
        const auto small_info = RL::CreateVoronoiWilderness(small, 0, true);
        Check(small_info.regions == 19, "automatic regions follow the smaller map's area");
        CheckBuildings(small, small_info);
        RL::CMap large(RL::Size(158, 100));
        const auto large_info = RL::CreateVoronoiWilderness(large, 0, true);
        Check(large_info.regions == 79, "quadrupling map area adds about four times as many regions");
        CheckBuildings(large, large_info);
    }
    RL::InitRandomness(42);
    RL::CMap roomy(RL::Size(120, 80));
    const auto roomy_info = RL::CreateVoronoiWilderness(roomy, 4, true);
    Check(roomy_info.regions == 4 && roomy_info.buildings.size() > 1,
          "an explicitly spacious town gets multiple small houses instead of a scaled-up building");
    CheckBuildings(roomy, roomy_info);
    const auto empty = RL::CreateVoronoiWilderness(roomy, 4, false);
    Check(empty.buildings.empty(), "building generation remains optional");
    RL::CMap huge(RL::Size(300, 300));
    const auto huge_info = RL::CreateVoronoiWilderness(huge);
    Check(huge_info.regions == 450, "automatic segmentation continues beyond the old 400-region ceiling");
    bool farmland = false, bog = false;
    for (std::size_t y = 0; y < huge.getHeight(); ++y) {
        for (std::size_t x = 0; x < huge.getWidth(); ++x) {
            const auto type = huge.get(x, y).getType();
            farmland |= type == "field" || type == "hedge";
            bog |= type == "bog_ground" || type == "bog_reeds" || type == "bog_pool";
        }
    }
    Check(!farmland && bog && huge_info.field_gates.empty(),
          "wilderness without buildings has bogs instead of cultivated fields and gates");
}

void TestVoronoiChasms()
{
    std::array<std::size_t, 3> counts{};
    std::array<std::uint64_t, 3> brightness{};
    for (unsigned seed = 0; seed < 8; ++seed) {
        RL::CMap map(RL::Size(120, 80));
        RL::InitRandomness(seed);
        RL::CreateVoronoiWilderness(map, 40, true);
        bool blocked = true;
        std::string snapshot;
        for (std::size_t y = 0; y < map.getHeight(); ++y) {
            for (std::size_t x = 0; x < map.getWidth(); ++x) {
                const auto& tile = map.get(x, y);
                const int layer = tile.getType() == "chasm_rim" ? 0 : tile.getType() == "chasm_wall" ? 1 :
                                  tile.getType() == "chasm" ? 2 : -1;
                if (layer >= 0) {
                    blocked &= !tile.isPassable();
                    ++counts[layer];
                    const auto colour = tile.getColor();
                    brightness[layer] += ((colour >> 16) & 0xFF) + ((colour >> 8) & 0xFF) + (colour & 0xFF);
                    Check(tile.getGlyph() != U'%', "ravines use rocky rims and subdued depth glyphs");
                }
                snapshot += std::format("{}:{}:{};", tile.getType(), tile.getColor(), static_cast<unsigned>(tile.getGlyph()));
            }
        }
        Check(blocked, "rims, descending faces and deep chasms remain impassable after connectivity repairs");
        RL::InitRandomness(seed);
        RL::CreateVoronoiWilderness(map, 40, true);
        std::string repeated;
        for (std::size_t y = 0; y < map.getHeight(); ++y) {
            for (std::size_t x = 0; x < map.getWidth(); ++x) {
                const auto& tile = map.get(x, y);
                repeated += std::format("{}:{}:{};", tile.getType(), tile.getColor(), static_cast<unsigned>(tile.getGlyph()));
            }
        }
        Check(snapshot == repeated, "ravine shapes, rim glyphs and depth colours reproduce from the seed");
    }
    Check(std::ranges::all_of(counts, [](auto count) { return count > 0; }), "ravines contain rims, descending faces and deeper pockets");
    if (std::ranges::all_of(counts, [](auto count) { return count > 0; })) {
        Check(brightness[0] / counts[0] > brightness[1] / counts[1] && brightness[1] / counts[1] > brightness[2] / counts[2],
              "ravines become progressively darker from the rim into the depths");
    }
}

void TestVoronoiFields()
{
    std::size_t checked = 0;
    for (unsigned seed = 0; seed < 64; ++seed) {
        RL::InitRandomness(seed);
        RL::CMap map(seed % 2 == 0 ? RL::Size(79, 50) : RL::Size(120, 80));
        const auto info = RL::CreateVoronoiWilderness(map, 0, seed % 3 != 0);
        if (info.field_gates.empty() || info.locations.empty()) { continue; }
        RL::Position root;
        if (!info.buildings.empty()) {
            root = info.buildings.front().door;
        } else {
            std::size_t best = std::numeric_limits<std::size_t>::max();
            for (std::size_t y = 0; y < map.getHeight(); ++y) {
                for (std::size_t x = 0; x < map.getWidth(); ++x) {
                    if (map.get(x, y).getType() == "corridor" && RL::Position(x, y).Distance(info.locations.front().center) < best) {
                        best = RL::Position(x, y).Distance(info.locations.front().center);
                        root = RL::Position(x, y);
                    }
                }
            }
        }
        Check(map.inside(root), "the village has a road anchor even without a house");
        if (!map.inside(root)) { continue; }
        std::vector<bool> seen(map.getWidth() * map.getHeight());
        std::vector<RL::Position> queue{root};
        seen[root.y * map.getWidth() + root.x] = true;
        for (std::size_t index = 0; index < queue.size(); ++index) {
            const auto at = queue[index];
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const RL::Position next(at.x + dx, at.y + dy);
                    if (!map.inside(next)) { continue; }
                    const auto cell = next.y * map.getWidth() + next.x;
                    const auto type = map.get(next).getType();
                    if (!seen[cell] && (type == "corridor" || type == "bridge" || type == "door_open" || type == "field" || type == "plant")) {
                        seen[cell] = true;
                        queue.push_back(next);
                    }
                }
            }
        }
        for (auto gate : info.field_gates) {
            ++checked;
            Check(map.get(gate).getType() == "door_open", "field gates remain entrances rather than being paved over");
            Check(seen[gate.y * map.getWidth() + gate.x], std::format("field entrance ({},{}) connects to village, seed {}", gate.x, gate.y, seed));
        }
        CheckBuildings(map, info);
    }
    Check(checked > 0, "generated fields exercise their village connections");
}

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
}

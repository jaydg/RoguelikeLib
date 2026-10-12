import rl.mapgenerators.forest;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

void TestForestRoads()
{
    std::size_t centres = 0, paved_centres = 0, base_roads = 0, extra_roads = 0;
    for (unsigned seed = 0; seed < 24; ++seed) {
        const RL::Size size(seed % 2 ? 120 : 79, seed % 2 ? 80 : 50);
        RL::CMap map(size), base(size), replay(size);
        RL::InitRandomness(seed);
        const auto locations = RL::GenerateForest(map, 0.7f, 0.05f, 5, 8, 1, 3);
        RL::InitRandomness(seed);
        RL::GenerateForest(base, 0.7f, 0.05f, 5, 0, 1, 3);
        RL::InitRandomness(seed);
        RL::GenerateForest(replay, 0.7f, 0.05f, 5, 8, 1, 3);
        std::vector<bool> seen(size.x * size.y);
        std::vector<RL::Position> queue{locations.front().center};
        seen[queue.front().y * size.x + queue.front().x] = true;
        for (std::size_t i = 0; i < queue.size(); ++i) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const RL::Position next(queue[i].x + dx, queue[i].y + dy);
                    if (!map.inside(next) || !map.get(next).isPassable()) continue;
                    const auto cell = next.y * size.x + next.x;
                    if (!seen[cell]) { seen[cell] = true; queue.push_back(next); }
                }
            }
        }
        bool connected = true, reproducible = true;
        for (const auto& clearing : locations) {
            connected &= seen[clearing.center.y * size.x + clearing.center.x] && map.get(clearing.center).isPassable();
            ++centres;
            paved_centres += map.get(clearing.center).getType() == "corridor";
        }
        for (std::size_t y = 0; y < size.y; ++y) {
            for (std::size_t x = 0; x < size.x; ++x) {
                reproducible &= map.get(x, y).getType() == replay.get(x, y).getType();
                base_roads += base.get(x, y).getType() == "corridor" || base.get(x, y).getType() == "bridge";
                extra_roads += map.get(x, y).getType() == "corridor" || map.get(x, y).getType() == "bridge";
            }
        }
        Check(connected, "clearing entrances stay reachable, including rivers and scattered rocks");
        Check(reproducible, "forest road networks reproduce from their seed");
    }
    Check(paved_centres * 10 < centres, "forest roads leave clearing centres predominantly unpaved");
    Check(extra_roads < base_roads * 2, "additional routes share substantial portions of the existing road network");
}

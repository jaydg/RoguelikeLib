import rl.maputils;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

void TestWindingRoad()
{
    RL::CMap map(RL::Size(80, 60));
    for (const auto& [start, end] : std::array{
            std::pair{RL::Position(10, 10), RL::Position(14, 10)},
            std::pair{RL::Position(10, 10), RL::Position(65, 10)},
            std::pair{RL::Position(10, 10), RL::Position(10, 50)},
            std::pair{RL::Position(65, 45), RL::Position(8, 12)},
            std::pair{RL::Position(0, 0), RL::Position(79, 59)},
            std::pair{RL::Position(20, 20), RL::Position(20, 20)}}) {
        RL::InitRandomness(42);
        std::vector<RL::Position> path;
        Check(RL::BuildWindingPath(path, map, start, end, 8), "winding paths terminate, including five-tile paths");
        Check(!path.empty() && path.front() == start && path.back() == end, "winding paths preserve both endpoints");
        bool continuous = true;
        for (std::size_t i = 0; i < path.size(); ++i) {
            continuous &= map.inside(path[i]);
            if (i > 0) {
                const auto dx = std::abs(static_cast<int>(path[i].x) - static_cast<int>(path[i - 1].x));
                const auto dy = std::abs(static_cast<int>(path[i].y) - static_cast<int>(path[i - 1].y));
                continuous &= std::max(dx, dy) <= 1;
            }
        }
        Check(continuous, "winding paths are connected and stay inside the map");
        RL::InitRandomness(42);
        std::vector<RL::Position> repeated;
        RL::BuildWindingPath(repeated, map, start, end, 8);
        Check(path == repeated, "winding paths reproduce from their seed");
    }
    std::vector<RL::Position> invalid{RL::Position(1, 1)};
    Check(!RL::BuildWindingPath(invalid, map, RL::Position(), RL::Position(10, 10), 8) && invalid.empty(),
          "invalid winding endpoints return false and clear the output");
    Check(!RL::BuildWindingPath(invalid, map, RL::Position(1, 1), RL::Position(10, 10), -1),
          "negative perturbation does not cause an unbounded loop");
    Check(RL::AddWindingRoad(map, RL::Position(5, 5), RL::Position(65, 45), 8) &&
          map.get(5, 5).getType() == "corridor" && map.get(65, 45).getType() == "corridor",
          "drawing a winding road uses the terminating path builder");
}

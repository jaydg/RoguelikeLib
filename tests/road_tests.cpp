import rl.maputils;
import rl.matrix;
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
    RL::CMap lake(RL::Size(24, 16));
    lake.Clear("grass");
    for (std::size_t y = 3; y < 13; ++y) for (std::size_t x = 7; x < 17; ++x)
        lake.SetCell(x, y, "water");
    const auto cost = [&lake](RL::Position p) { return lake.get(p).getType() == "water" ? 0 : 2; };
    const RL::Position start(1, 8), end(22, 8);
    RL::InitRandomness(42);
    std::vector<RL::Position> route;
    Check(RL::BuildWindingRoute(route, lake, start, end, 12, cost) &&
          route.front() == start && route.back() == end,
          "terrain-aware winding routes preserve endpoints around a lake");
    bool safe = !route.empty();
    for (std::size_t i = 0; i < route.size(); ++i) {
        safe &= cost(route[i]) > 0;
        if (i > 0) safe &= std::max(std::abs(int(route[i].x) - int(route[i - 1].x)),
                                   std::abs(int(route[i].y) - int(route[i - 1].y))) == 1;
    }
    Check(safe, "terrain-aware winding routes remain continuous without crossing blocked water");
    for (std::size_t y = 0; y < lake.getHeight(); ++y) lake.SetCell(12, y, "water");
    Check(!RL::BuildWindingRoute(route, lake, start, end, 12, cost) && route.empty(),
          "a blocked terrain-aware route fails cleanly rather than bridging a lake");
    Check(!RL::BuildWindingRoute(route, lake, RL::Position(12, 8), end, 12, cost),
          "terrain-aware winding routes reject a blocked endpoint");
}

void TestRoadsWithBridges()
{
    // 1. FindStraightBridgeSpan unit tests
    {
        RL::CMap map(RL::Size(30, 20));
        map.Clear("grass");
        // Vertical river from x=10 to x=12 across all y
        for (std::size_t y = 0; y < 20; ++y) {
            for (std::size_t x = 10; x <= 12; ++x) {
                map.SetCell(x, y, "water");
            }
        }

        // Test valid horizontal crossing
        auto span = RL::FindStraightBridgeSpan(map, RL::Position(9, 5), 1, 0, 10);
        Check(span.valid, "valid horizontal bridge span detected across river");
        Check(span.span_length == 3, "span length is 3 water cells");
        Check(span.landing == RL::Position(13, 5), "landing position is on opposite bank");

        // Test valid diagonal crossing
        auto diag_span = RL::FindStraightBridgeSpan(map, RL::Position(9, 5), 1, 1, 10);
        Check(diag_span.valid, "valid diagonal bridge span detected across river");
        Check(diag_span.span_length == 3, "diagonal span length is 3 water cells");
        Check(diag_span.landing == RL::Position(13, 9), "diagonal landing is opposite bank");

        // Test starting from water fails
        auto from_water = RL::FindStraightBridgeSpan(map, RL::Position(10, 5), 1, 0, 10);
        Check(!from_water.valid, "cannot start bridge from water");

        // Test max_span cutoff
        auto too_long = RL::FindStraightBridgeSpan(map, RL::Position(9, 5), 1, 0, 2);
        Check(!too_long.valid, "bridge exceeding max_span is invalid");

        // Test out of bounds / running off map
        RL::CMap water_edge(RL::Size(20, 20));
        water_edge.Clear("grass");
        for (std::size_t x = 15; x < 20; ++x) {
            for (std::size_t y = 0; y < 20; ++y) water_edge.SetCell(x, y, "water");
        }
        auto off_map = RL::FindStraightBridgeSpan(water_edge, RL::Position(14, 5), 1, 0, 10);
        Check(!off_map.valid, "bridge running off map without opposite bank is invalid");
    }

    // 2. BuildRouteWithBridges & AddRoadWithBridges
    {
        RL::CMap map(RL::Size(40, 20));
        map.Clear("grass");
        // Vertical river at x=18..20
        for (std::size_t y = 0; y < 20; ++y) {
            for (std::size_t x = 18; x <= 20; ++x) {
                map.SetCell(x, y, "water");
            }
        }

        std::vector<RL::Position> route;
        RL::Position start(5, 10), end(35, 10);
        bool found = RL::BuildRouteWithBridges(route, map, start, end);
        Check(found, "route across river found with straight bridge");
        Check(!route.empty() && route.front() == start && route.back() == end, "endpoints preserved");

        // Check continuity of the route
        bool continuous = true;
        for (std::size_t i = 1; i < route.size(); ++i) {
            const int dx = std::abs(static_cast<int>(route[i].x) - static_cast<int>(route[i - 1].x));
            const int dy = std::abs(static_cast<int>(route[i].y) - static_cast<int>(route[i - 1].y));
            if (std::max(dx, dy) > 1) {
                continuous = false;
                break;
            }
        }
        Check(continuous, "bridge-aware route is continuous 8-connected path");

        // Test AddRoadWithBridges
        RL::CMatrix<bool> road_mask(map.getSize(), false);
        bool painted = RL::AddRoadWithBridges(map, start, end, "corridor", "bridge", &road_mask);
        Check(painted, "AddRoadWithBridges painted path and bridge");
        Check(map.get(5, 10).getType() == "corridor", "start is paved corridor");
        Check(map.get(35, 10).getType() == "corridor", "end is paved corridor");
        Check(map.get(18, 10).getType() == "bridge", "river tile 18 is paved bridge");
        Check(map.get(19, 10).getType() == "bridge", "river tile 19 is paved bridge");
        Check(map.get(20, 10).getType() == "bridge", "river tile 20 is paved bridge");
        Check(road_mask.get(19, 10), "road_mask reflects bridge tile");

        // Test road reuse with existing_roads
        RL::Position start2(5, 15), end2(35, 10);
        std::vector<RL::Position> route2;
        RL::SRouteWithBridgesConfig config{ .existing_roads = &road_mask };
        Check(RL::BuildRouteWithBridges(route2, map, start2, end2, config), "second route connects");
        bool joined_existing = false;
        for (const auto& p : route2) {
            if (p != end2 && road_mask.get(p.x, p.y)) {
                joined_existing = true;
                break;
            }
        }
        Check(joined_existing, "second route reuses existing road network");
    }
}

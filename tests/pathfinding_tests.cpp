// Tests for RoguelikeLib's rl.pathfinding

import rl.map;
import rl.pathfinding;
import rl.position;
import rltests.check;
import std;

using RL::tests::Check;

void TestPathfinding()
{
    // 1. Simple straight line on open map
    const RL::CMap open = RL::tests::MakeMap({
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
    });

    {
        std::vector<RL::Position> path;
        const bool found = RL::FindPath(open, RL::Position(0, 0), RL::Position(4, 0), path, false);
        Check(found, "straight horizontal path found");
        Check(path.size() == 4, "straight horizontal path length is 4");
        Check(!path.empty() && path.back() == RL::Position(4, 0), "path ends at destination");
        Check(!path.empty() && path.front() == RL::Position(1, 0), "path starts at first step");
    }

    // 2. Cardinal moves only (diagonals = false)
    {
        std::vector<RL::Position> path;
        const bool found = RL::FindPath(open, RL::Position(0, 0), RL::Position(2, 2), path, false);
        Check(found, "cardinal path found to (2,2)");
        Check(path.size() == 4, "Manhattan distance is 4 without diagonals");
        Check(!path.empty() && path.back() == RL::Position(2, 2), "cardinal path reaches destination");

        // Verify every step is cardinal (dx + dy == 1)
        RL::Position current(0, 0);
        bool all_cardinal = true;
        for (const auto& step : path) {
            const int dx = std::abs(static_cast<int>(step.x) - static_cast<int>(current.x));
            const int dy = std::abs(static_cast<int>(step.y) - static_cast<int>(current.y));
            if (dx + dy != 1) {
                all_cardinal = false;
            }
            current = step;
        }
        Check(all_cardinal, "all steps without diagonals are strictly cardinal");
    }

    // 3. Diagonal moves allowed (diagonals = true)
    {
        std::vector<RL::Position> path;
        const bool found = RL::FindPath(open, RL::Position(0, 0), RL::Position(3, 3), path, true);
        Check(found, "diagonal path found to (3,3)");
        Check(path.size() == 3, "Chebyshev distance is 3 with diagonals");
        Check(!path.empty() && path.back() == RL::Position(3, 3), "diagonal path reaches destination");
    }

    // 4. Navigating around a wall
    const RL::CMap walled = RL::tests::MakeMap({
        ".....",
        ".TTT.",
        "...T.",
        ".T.T.",
        ".T...",
    });

    {
        std::vector<RL::Position> path;
        const bool found = RL::FindPath(walled, RL::Position(0, 0), RL::Position(2, 2), path, true);
        Check(found, "path around walls found");
        Check(!path.empty() && path.back() == RL::Position(2, 2), "wall avoidance reaches goal");

        // Verify no step lands on a tree ('T')
        bool walked_on_wall = false;
        for (const auto& step : path) {
            if (walled.get(step).getType() == "tree") {
                walked_on_wall = true;
            }
        }
        Check(!walked_on_wall, "path does not step onto impassable tiles");
    }

    // 5. Completely blocked destination
    const RL::CMap blocked = RL::tests::MakeMap({
        ".....",
        ".TTT.",
        ".T.T.",
        ".TTT.",
        ".....",
    });

    {
        std::vector<RL::Position> path;
        const bool found = RL::FindPath(blocked, RL::Position(0, 0), RL::Position(2, 2), path, true);
        Check(!found, "unreachable goal returns false");
        Check(path.empty(), "unreachable goal leaves path empty");
    }

    // 6. Start is already at destination
    {
        std::vector<RL::Position> path;
        const bool found = RL::FindPath(open, RL::Position(1, 1), RL::Position(1, 1), path, true);
        Check(found, "already at goal returns true");
        Check(path.empty(), "already at goal has empty path");
    }

    // 7. Bresenham line and visitor
    {
        const RL::Position from(0, 0);
        const RL::Position to(4, 2);

        // Vector overload
        const auto line = from.BuildBresenhamLine(to);
        Check(line.size() == 5, "Bresenham line has 5 points from (0,0) to (4,2)");
        Check(!line.empty() && line.front() == from && line.back() == to, "Bresenham line endpoints match");

        // Buffer overload
        std::vector<RL::Position> buffer;
        from.BuildBresenhamLine(to, buffer);
        Check(buffer == line, "BuildBresenhamLine with buffer produces identical points");

        // Visitor with early stopping
        std::vector<RL::Position> visited;
        from.ForEachBresenham(to, [&](const RL::Position& p) {
            visited.push_back(p);
            return visited.size() < 3; // stop after 3 points
        });
        Check(visited.size() == 3, "ForEachBresenham stops early when callback returns false");
        Check(visited[0] == line[0] && visited[1] == line[1] && visited[2] == line[2],
              "ForEachBresenham points match line prefix");
    }

    // 8. Position constexpr and noexcept guarantees
    {
        constexpr RL::Position def;
        static_assert(def.x == RL::Position::invalid && def.y == RL::Position::invalid);
        constexpr RL::Position pos(10, 20);
        static_assert(pos.x == 10 && pos.y == 20);
        static_assert(pos == RL::Position(10, 20));
        static_assert(pos != RL::Position(10, 21));
        static_assert(noexcept(RL::Position()));
        static_assert(noexcept(RL::Position(1, 2)));
        static_assert(noexcept(pos == pos));
        static_assert(noexcept(pos != pos));
        static_assert(std::is_trivially_copyable_v<RL::Position>);

        Check(pos.x == 10 && pos.y == 20, "constexpr Position constructed correctly");
    }

    // 9. std::hash specialization for Position
    {
        std::unordered_set<RL::Position> set;
        set.insert(RL::Position(1, 2));
        set.insert(RL::Position(3, 4));
        set.insert(RL::Position(1, 2));

        Check(set.size() == 2, "unordered_set deduplicates Positions correctly");
        Check(set.contains(RL::Position(1, 2)), "unordered_set finds inserted Position");
        Check(!set.contains(RL::Position(5, 6)), "unordered_set does not find missing Position");

        std::unordered_map<RL::Position, int> map;
        map[RL::Position(10, 20)] = 42;
        Check(map[RL::Position(10, 20)] == 42, "unordered_map stores values at Position key");
    }
}

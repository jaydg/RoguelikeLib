//////////////////////////////////////////////////////////////////////////
// Path finding
//////////////////////////////////////////////////////////////////////////
module;

export module rl.pathfinding;

import rl.map;
import rl.maputils;
import rl.matrix;
import rl.position;
import std;

export namespace RL {

    // CMap &level - in/out
    // input level must be in form of values (higher than floodfill distance)
    // - LevelElementCorridor_value
    // - LevelElementWall_value
    // as output you get a flood filled level
    bool FindPath(
        CMap &level,
        const Position& start,
        const Position& end,
        std::vector<Position>& path,
        const bool& diagonals = true)
    {
        auto pathmap = CMatrix<int>(level.getSize(), false);
        for (auto x = 0; x <  level.GetWidth(); x++) {
            for (auto y = 0; y < level.GetHeight(); y++) {
                bool passable = level.GetCell(x, y) != LevelElementWall && level.GetCell(x, y) != LevelElementDoorClose;
                pathmap.set(x, y,
                    passable ? LevelElementCorridor_value : LevelElementWall_value);
            }
        }

        // fill from end to start
        if (!pathmap.FloodFill(end, 0, false, 1, start)) {
            return false;
        }

        // walk from start to end
        Position pos = start;
        Position new_pos = start;

        while (true) {
            if (pos == end) {
                return true;
            }

            pos = new_pos;

            if (pos != start) {
                path.push_back(pos);
            }

            int current_value = pathmap(pos.x, pos.y);

            if (diagonals) {
                if (pos.x > 0 && pos.y > 0)
                    if (pathmap(pos.x - 1, pos.y - 1) < current_value) { // NW
                        new_pos.x--;
                        new_pos.y--;
                        continue;
                    }

                if (pos.x < pathmap.getWidth() - 1 && pos.y > 0)
                    if (pathmap(pos.x + 1, pos.y - 1) < current_value) { // NE
                        new_pos.x++;
                        new_pos.y--;
                        continue;
                    }

                if (pos.x < pathmap.getWidth() - 1 && pos.y < pathmap.getHeight() - 1)
                    if (pathmap(pos.x + 1, pos.y + 1) < current_value) { // SE
                        new_pos.x++;
                        new_pos.y++;
                        continue;
                    }

                if (pos.x > 0 && pos.y < pathmap.getHeight() - 1)
                    if (pathmap(pos.x - 1, pos.y + 1) < current_value) { // SW
                        new_pos.x--;
                        new_pos.y++;
                        continue;
                    }
            }

            if (pos.y > 0)
                if (pathmap(pos.x, pos.y - 1) < current_value) { // N
                    new_pos.y--;
                    continue;
                }

            if (pos.x < pathmap.getWidth() - 1)
                if (pathmap(pos.x + 1, pos.y) < current_value) { // E
                    new_pos.x++;
                    continue;
                }

            if (pos.x > 0 && pos.y < pathmap.getHeight() - 1)
                if (pathmap(pos.x, pos.y + 1) < current_value) { // S
                    new_pos.y++;
                    continue;
                }

            if (pos.x > 0 && pos.y > 0)
                if (pathmap(pos.x - 1, pos.y) < current_value) { // W
                    new_pos.x--;
                    continue;
                }

            if (pos == new_pos && new_pos != end) {
                path.clear();
                return false;
            }
        }
    }

} // namespace RL

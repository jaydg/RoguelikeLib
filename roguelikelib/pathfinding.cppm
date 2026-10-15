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

export namespace RL
{

// Finds a path from start to end on the level.
// Returns true and writes the sequential steps (excluding start) to path if reachable.
bool FindPath(
    const CMap &level,
    const Position& start,
    const Position& end,
    std::vector<Position>& path,
    bool diagonals = true)
{
    auto pathmap = CMatrix<int>(level.getSize(), std::numeric_limits<int>::max());

    for (std::size_t y = 0; y < level.getHeight(); y++) {
        for (std::size_t x = 0; x < level.getWidth(); x++) {
            if (level.get(x, y).isPassable()) {
                pathmap.set(x, y, std::numeric_limits<int>::max() - 1);
            }
        }
    }

    // fill from end to start, with the same moves the walk back may take
    if (!pathmap.FloodFill(end, 0, diagonals, 1, start)) {
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

        int current_value = pathmap.get(pos.x, pos.y);

        if (diagonals) {
            if (pos.x > 0 && pos.y > 0)
                if (pathmap.get(pos.x - 1, pos.y - 1) < current_value) { // NW
                    new_pos.x--;
                    new_pos.y--;
                    continue;
                }

            if (pos.x < pathmap.getWidth() - 1 && pos.y > 0)
                if (pathmap.get(pos.x + 1, pos.y - 1) < current_value) { // NE
                    new_pos.x++;
                    new_pos.y--;
                    continue;
                }

            if (pos.x < pathmap.getWidth() - 1 && pos.y < pathmap.getHeight() - 1)
                if (pathmap.get(pos.x + 1, pos.y + 1) < current_value) { // SE
                    new_pos.x++;
                    new_pos.y++;
                    continue;
                }

            if (pos.x > 0 && pos.y < pathmap.getHeight() - 1)
                if (pathmap.get(pos.x - 1, pos.y + 1) < current_value) { // SW
                    new_pos.x--;
                    new_pos.y++;
                    continue;
                }
        }

        if (pos.y > 0)
            if (pathmap.get(pos.x, pos.y - 1) < current_value) { // N
                new_pos.y--;
                continue;
            }

        if (pos.x < pathmap.getWidth() - 1)
            if (pathmap.get(pos.x + 1, pos.y) < current_value) { // E
                new_pos.x++;
                continue;
            }

        if (pos.y < pathmap.getHeight() - 1)
            if (pathmap.get(pos.x, pos.y + 1) < current_value) { // S
                new_pos.y++;
                continue;
            }

        if (pos.x > 0)
            if (pathmap.get(pos.x - 1, pos.y) < current_value) { // W
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

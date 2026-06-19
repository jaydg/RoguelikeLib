module;

export module rl.mapgenerators.caves;

import rl.map;
import rl.maputils;
import rl.position;
import rl.randomness;
import rl.tile;
import std;

export namespace RL
{

// create a game of life cave
void CreateCaves(CMap &level, int iterations = 1, float density = 0.65)
{
    if (level.getWidth() == 0 || level.getHeight() == 0) {
        return;
    }

    level.Clear("room");

    for (int fill = 0; fill < static_cast<int>(static_cast<float>(level.getWidth() * level.getHeight()) * density); fill++) {
        level.SetCell(Random(level.getWidth()), Random(level.getHeight()), "wall");
    }

    for (int iteration = 0; iteration < iterations; iteration++) {
        for (std::size_t x = 0; x < level.getWidth(); x++) {
            for (std::size_t y = 0; y < level.getHeight(); y++) {
                int neighbours = level.CountNeighbors(Position(x, y), CTile::ByType("wall"));

                if (level.get(x, y).getType() == "wall") {
                    if (neighbours < 4) {
                        level.SetCell(x, y, "wall");
                    }
                } else {
                    if (neighbours > 4) {
                        level.SetCell(x, y, "wall");
                    }
                }

                if (x == 0 || x == level.getWidth() - 1 || y == 0 || y == level.getHeight() - 1) {
                    level.SetCell(x, y, "wall");
                }
            }
        }
    }

    ConnectClosestRooms(level, true);
}

} // end of namespace RL

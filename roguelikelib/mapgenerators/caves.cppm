module;

export module rl.mapgenerators.caves;

import rl.map;
import rl.maputils;
import rl.position;
import rl.randomness;
import std;

export namespace RL {

// create a game of life cave
void CreateCaves(CMap &level, int iterations = 1, float density = 0.65)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear(LevelElementRoom);

    for(int fill = 0; fill < static_cast<int>(static_cast<float>(level.GetWidth() * level.GetHeight()) * density); fill++) {
        level.SetCell(Random(level.GetWidth()), Random(level.GetHeight()), LevelElementWall);
    }

    for(int iteration = 0; iteration < iterations; iteration++) {
        for (std::size_t x = 0; x < level.GetWidth(); x++) {
            for(std::size_t y = 0; y < level.GetHeight(); y++) {
                int neighbours = CountNeighboursOfType(level, LevelElementWall, Position(x, y));

                if(level.GetCell(x, y) == LevelElementWall) {
                    if(neighbours < 4) {
                        level.SetCell(x, y, LevelElementRoom);
                    }
                } else {
                    if(neighbours > 4) {
                        level.SetCell(x, y, LevelElementWall);
                    }
                }

                if(x == 0 || x == level.GetWidth() - 1 || y == 0 || y == level.GetHeight() - 1) {
                    level.SetCell(x, y, LevelElementWall);
                }
            }
        }
    }

    ConnectClosestRooms(level, true);
    ConvertValuesToTiles(level);
}

} // end of namespace RL

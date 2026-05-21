module;

export module rl.mapgenerators.standarddungeon;

import rl.map;
import rl.maputils;
import rl.position;
import rl.randomness;
import std;

export namespace RL {

void CreateStandardDungeon(CMap &level, int max_number_of_rooms, bool with_doors = true) {
    if (level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear();

    Position p;
    Size room_size;

    // place rooms
    for (int room_number = 0; room_number < max_number_of_rooms; ++room_number) {
        // size of room
        room_size.x = Random(5) + 8;
        room_size.y = Random(5) + 5;

        if (FindOnMapRandomRectangleOfType(level, LevelElementWall, p, room_size)) {
            for (std::size_t x = 1; x < room_size.x - 1; x++) {
                for (std::size_t y = 1; y < room_size.y - 1; y++) {
                    level.SetCell(p.x + x, p.y + y, LevelElementRoom);
                }
            }
        }
    }

    ConnectClosestRooms(level, true, true);
    ConvertValuesToTiles(level);

    if (with_doors) {
        AddDoors(level, 1, 0.5);
    }
}

} // namespace RL

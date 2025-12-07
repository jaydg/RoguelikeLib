#pragma once
#ifndef RL_STANDARDDUNGEON_H
#define RL_STANDARDDUNGEON_H

#include "../map.h"
#include "../randomness.h"
#include "../maputils.h"

namespace RL {
inline
void CreateStandardDungeon(CMap &level, int max_number_of_rooms = 10, bool with_doors = true)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear();

    Position p;
    Size room_size;

    // place rooms
    for(int room_number = 0; room_number < max_number_of_rooms; ++room_number) {
        // size of room
        room_size.x = Random(5) +8;
        room_size.y = Random(5) +5;

        if(FindOnMapRandomRectangleOfType(level, LevelElementWall, p, room_size)) {
            for(int x = 1; x < room_size.x - 1; x++)
                for(int y = 1; y < room_size.y - 1; y++) {
                    level.SetCell(p.x + x, p.y + y, LevelElementRoom);
                }
        }
    }

    ConnectClosestRooms(level, true, true); // changes tiles to values
    ConvertValuesToTiles(level);

    if(with_doors) {
        AddDoors(level, 1, 0.5);
    }
}

} // end of namespace RL

#endif

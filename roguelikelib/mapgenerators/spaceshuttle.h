#pragma once
#ifndef RL_SPACESHUTTLE_H
#define RL_SPACESHUTTLE_H

#include "../map.h"
#include "../randomness.h"
#include "../maputils.h"

namespace RL {
inline
void CreateSpaceShuttle(CMap &level, const int& max_number_of_rooms = 15, bool mirror_vertical = false)
{
    const int room_min_size = 3;
    const int room_max_size = 15;

    int number_of_rooms;
    int free_cells;

    std::list < SRoom > list_of_rooms;
    std::list < SRoom >::iterator m, _m;
    int x, y;

    int x1, y1, x2, y2, rx, ry;

    while(1) { // create until shuttle looks good
        list_of_rooms.clear();

        // fill with walls
        for(x = 0; x < (int) level.GetWidth(); ++x)
            for(y = 0; y < (int) level.GetHeight(); ++y) {
                level.SetCell(x, y, LevelElementWall_value);
            }

        for(number_of_rooms = 0; number_of_rooms < max_number_of_rooms;) {
            if(number_of_rooms == 0) {
                x1 = (int) level.GetWidth() / 2 - Random(room_max_size);
                y1 = (int) level.GetHeight() / 2 - Random(room_max_size) - room_min_size;
                rx = Random(room_max_size) + room_min_size;
                ry = Random(room_max_size - room_min_size) + room_min_size;
                x2 = x1 + rx;
                y2 = (int) level.GetHeight() / 2;

                if(x2 >= (int) level.GetWidth()) {
                    continue;
                }
            } else {
                x1 = Random((int) level.GetWidth() - room_min_size) + 1;
                y1 = Random((int) level.GetHeight() - room_min_size) / 2 + 1;
                rx = Random(room_max_size - room_min_size) + room_min_size;
                ry = Random(room_max_size - room_min_size) + room_min_size;
                x2 = x1 + rx;
                y2 = y1 + ry;

                if(x2 >= (int) level.GetWidth() - 1 || y2 >= (int) level.GetHeight() / 2 + 3) {
                    continue;
                }
            }

            // is in existing room?
            bool rand_again = false;

            for(m = list_of_rooms.begin(), _m = list_of_rooms.end(); m != _m; m++) {
                SRoom &room = *m;
                rand_again = true;

                if(room.IsInRoom(x1, y1)) {
                    if(!room.IsInRoom(x2, y2)) {
                        rand_again = false;
                        room.type++;
                    }

                    break;
                }

                if(room.IsInRoom(x2, y2)) {
                    if(!room.IsInRoom(x1, y1)) {
                        rand_again = false;
                        room.type++;
                    }

                    break;
                }
            }

            if(rand_again) {
                continue;
            }

            // Create room
            SRoom new_room;
            new_room.corner1.x = x1;
            new_room.corner1.y = y1;
            new_room.corner2.x = x2;
            new_room.corner2.y = y2;

            if(number_of_rooms == 0) {
                new_room.type = 0;
            }

            list_of_rooms.push_back(new_room);
            number_of_rooms++;
        } // end of for

        // create mirror
        m = list_of_rooms.begin();
        int index;

        for(index = 0; index < number_of_rooms; index++, m++) {
            SRoom room = *m;

            if(mirror_vertical) {
                room.corner1.x = (int) level.GetWidth() - room.corner1.x - 1;
                room.corner2.x = (int) level.GetWidth() - room.corner2.x - 1;
                x1 = room.corner1.x;
                room.corner1.x = room.corner2.x;
                room.corner2.x = x1;
            } else {
                room.corner1.y = (int) level.GetHeight() - room.corner1.y - 1;
                room.corner2.y = (int) level.GetHeight() - room.corner2.y - 1;
                y1 = room.corner1.y;
                room.corner1.y = room.corner2.y;
                room.corner2.y = y1;
            }

            list_of_rooms.insert(m, room);
        };

        for(m = list_of_rooms.begin(), _m = list_of_rooms.end(); m != _m; m++) {
            SRoom &room = *m;

            for(x = room.corner1.x; x <= room.corner2.x; x++)
                for(y = room.corner1.y; y <= room.corner2.y; y++) {
                    if(level.GetCell(x, y) == LevelElementWall_value) {
                        level.SetCell(x, y, room.type);
                    }
                }
        }

        // Create walls on connections
        free_cells = 0;

        for(x = 0; x < (int) level.GetWidth() - 1; x++) {
            for(y = 0; y < (int) level.GetHeight() / 2; y++) {
                if(level.GetCell(x, y) != level.GetCell(x + 1, y) && level.GetCell(x + 1, y) != LevelElementWall_value) {
                    level.SetCell(x, y, LevelElementWall_value);
                } else if(level.GetCell(x, y) != level.GetCell(x, y + 1) && level.GetCell(x, y + 1) != LevelElementWall_value) {
                    level.SetCell(x, y, LevelElementWall_value);
                } else if(level.GetCell(x, y) != level.GetCell(x + 1, y + 1) && level.GetCell(x + 1, y + 1) != LevelElementWall_value) {
                    level.SetCell(x, y, LevelElementWall_value);
                }

                if(level.GetCell(x, y) != LevelElementWall_value) {
                    free_cells += 2; // +2 for mirror
                }

                level.SetCell(x, level.GetHeight() - y - 1, level.GetCell(x, y)); // i odbicie lustrzane
            }
        }

        // Size of ship
        if(free_cells < (int) level.GetHeight() * (int) level.GetWidth() / 4) {
            continue;
        }

        ConvertValuesToTiles(level);
        ConnectClosestRooms(level, true);
        break;
    }

    ConvertValuesToTiles(level);
    AddDoors(level, 1, 0);
}

} // end of namespace RL

#endif

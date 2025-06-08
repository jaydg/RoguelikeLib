#pragma once
#ifndef RL_MINES_H
#define RL_MINES_H

#include "../map.h"
#include "../randomness.h"
#include "../maputils.h"
#include <list>

namespace RL {
inline
void CreateMines(CMap &level, int max_number_of_rooms = 10)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear();

    int x, y, sx, sy;

    typedef std::list < SRoom > list_of_rooms;

    list_of_rooms :: iterator m;
    list_of_rooms rooms;
    SRoom room;

    int random_number;
    int diff_x, diff_y;

    Position p, p1, p2;

    // Place rooms
    for(int room_number = 0; room_number < max_number_of_rooms; ++room_number) {
        // size of room
        sx = Random(5) +6;
        sy = Random(5) +6;

        if(FindOnMapRandomRectangleOfType(level, LevelElementWall, p, RL::Size(sx + 4, sy + 4))) {
            p.x += 2;
            p.y += 2;

            // Connect the room to existing one
            if(rooms.size() > 0) {

                random_number = Random(static_cast<int>(rooms.size()));

                for(m = rooms.begin(); --random_number > 0; ++m);

                // center of this room
                p1.x = p.x + sx / 2;
                p1.y = p.y + sy / 2;

                // center of second room
                p2.x = (*m).corner1.x + ((*m).corner2.x - (*m).corner1.x) / 2;
                p2.y = (*m).corner1.y + ((*m).corner2.y - (*m).corner1.y) / 2;

                // found the way to connect rooms
                diff_x = p2.x - p1.x;
                diff_y = p2.y - p1.y;

                if(diff_x < 0) {
                    diff_x = -diff_x;
                }

                if(diff_y < 0) {
                    diff_y = -diff_y;
                }

                x = p1.x;
                y = p1.y;

                while(!(diff_x == 0 && diff_y == 0)) {
                    // move horizontally
                    if(RandomLowerThatLimit(diff_x, diff_x + diff_y)) {
                        diff_x--;

                        if(x > p2.x) {
                            x--;
                        } else {
                            x++;
                        }
                    } else {
                        diff_y--;

                        if(y > p2.y) {
                            y--;
                        } else {
                            y++;
                        }
                    }

                    // Check what is on that position
                    if(level.GetCell(x, y) == LevelElementRoom) {
                        break;
                    } else if(level.GetCell(x, y) == LevelElementCorridor)
                        if(CoinToss()) {
                            break;
                        }

                    level.SetCell(x, y, LevelElementCorridor);
                }
            }

            // add to list of rooms
            room.corner1.x = p.x;
            room.corner1.y = p.y;
            room.corner2.x = p.x + sx;
            room.corner2.y = p.y + sy;
            room.type = room_number;
            rooms.push_back(room);

            // draw_room
            int room_type = Random(4);

            if(sx == sy) {
                room_type = 3;
            }

            if(room_type != 2) {
                for(y = 0; y < sy; y++)
                    for(x = 0; x < sx; x++) {
                        switch(room_type) {
                        case 0: // rectangle room
                        case 1:
                            level.SetCell(p.x + x, p.y + y, LevelElementRoom);
                            break;

                        case 3: // round room
                            if(Distance(sx / 2, sx / 2, x, y) < sx / 2) {
                                level.SetCell(p.x + x, p.y + y, LevelElementRoom);
                            }

                            break;
                        }
                    }
            } // end if
            else { // typ==2 - Diamond
                for(y = 0; y <= sy / 2; y++)
                    for(x = 0; x <= sx / 2; x++) {
                        if(y >= x) {
                            level.SetCell(p.x + x + sx / 2, p.y + y, LevelElementRoom);
                            level.SetCell(p.x + x + sx / 2, p.y + sy - y, LevelElementRoom);
                            level.SetCell(p.x + sx / 2 - x, p.y + y, LevelElementRoom);
                            level.SetCell(p.x + sx / 2 - x, p.y + sy - y, LevelElementRoom);
                        }
                    }
            }
        } // end of room addition
    }
};

} // end of namespace RL

#endif

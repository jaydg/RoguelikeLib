module;

export module rl.mapgenerators.mines;

import rl.distance;
import rl.map;
import rl.maputils;
import rl.position;
import rl.randomness;
import std;

export namespace RL {

void CreateMines(CMap &level, int max_number_of_rooms = 10)
{
    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    level.Clear();

    typedef std::list <SRoom> list_of_rooms;

    list_of_rooms :: iterator m;
    list_of_rooms rooms;
    SRoom room;

    Position p, p1, p2;

    // Place rooms
    for(int room_number = 0; room_number < max_number_of_rooms; ++room_number) {
        // size of room
        const std::size_t sx = Random(5) + 6;
        const std::size_t sy = Random(5) + 6;

        if(FindOnMapRandomRectangleOfType(level, "wall", p, Size(sx + 4, sy + 4))) {
            p.x += 2;
            p.y += 2;

            // Connect the room to existing one
            if(!rooms.empty()) {
                std::size_t random_number = Random(rooms.size());

                for(m = rooms.begin(); ; ++m) {
                    if(random_number == 0) {
                        break;
                    }
                    random_number--;
                }

                // center of this room
                p1.x = p.x + sx / 2;
                p1.y = p.y + sy / 2;

                // center of second room
                p2.x = m->corner1.x + (m->corner2.x - m->corner1.x) / 2;
                p2.y = m->corner1.y + (m->corner2.y - m->corner1.y) / 2;

                // found the way to connect rooms
                std::size_t diff_x = diff(p1.x, p2.x);
                std::size_t diff_y = diff(p1.y, p2.y);

                std::size_t x = p1.x;
                std::size_t y = p1.y;

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
                    if(level.GetCell(x, y).getType() == "room") {
                        break;
                    } else if(level.GetCell(x, y).getType() == "corridor")
                        if(CoinToss()) {
                            break;
                        }

                    level.SetCell(x, y, "corridor");
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
            int room_type = static_cast<int>(Random(4));

            if(sx == sy) {
                room_type = 3;
            }

            if(room_type != 2) {
                for(std::size_t y = 0; y < sy; y++)
                    for(std::size_t x = 0; x < sx; x++) {
                        switch(room_type) {
                        case 0: // rectangle room
                        case 1:
                            level.SetCell(p.x + x, p.y + y, "room");
                            break;

                        case 3: // round room
                        default:
                            if(Distance(sx / 2, sx / 2, x, y) < sx / 2) {
                                level.SetCell(p.x + x, p.y + y, "room");
                            }

                            break;
                        }
                    }
            } // end if
            else { // typ==2 - Diamond
                for(std::size_t y = 0; y <= sy / 2; y++)
                    for(std::size_t x = 0; x <= sx / 2; x++) {
                        if(y >= x) {
                            level.SetCell(p.x + x + sx / 2, p.y + y, "room");
                            level.SetCell(p.x + x + sx / 2, p.y + sy - y, "room");
                            level.SetCell(p.x + sx / 2 - x, p.y + y, "room");
                            level.SetCell(p.x + sx / 2 - x, p.y + sy - y, "room");
                        }
                    }
            }
        } // end of room addition
    }
}

} // end of namespace RL

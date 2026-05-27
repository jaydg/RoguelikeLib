module;

export module rl.mapgenerators.spaceshuttle;

import rl.map;
import rl.randomness;
import rl.maputils;
import rl.matrix;
import std;

export namespace RL {

void CreateSpaceShuttle(CMap &level, const int& max_number_of_rooms = 15)
{
    constexpr int room_max_size = 15;
    constexpr int room_min_size = 3;

    int number_of_rooms;

    std::list<SRoom> list_of_rooms;
    std::list<SRoom>::iterator m;

    // create until shuttle looks good
    while (true) {
        // drafting board - contains 0 for solid matter and the room ID of each room
        CMatrix<int> id_matrix(level.getSize(), 0);
        list_of_rooms.clear();

        // Create rooms (upper half)
        for (number_of_rooms = 0; number_of_rooms < max_number_of_rooms;) {
            std::size_t x1, y1, x2, y2;
            std::size_t rx, ry;

            if (number_of_rooms == 0) {
                x1 = level.GetWidth() / 2 - Random(room_max_size);
                y1 = level.GetHeight() / 2 - Random(room_max_size) - room_min_size;
                rx = Random(room_max_size) + room_min_size;
                ry = Random(room_max_size - room_min_size) + room_min_size;
                x2 = x1 + rx;
                y2 = level.GetHeight() / 2;

                if (x2 >= level.GetWidth()) {
                    continue;
                }
            } else {
                x1 = Random(static_cast<int>(level.GetWidth()) - room_min_size) + 1;
                y1 = Random(static_cast<int>(level.GetHeight()) - room_min_size) / 2 + 1;
                rx = Random(room_max_size - room_min_size) + room_min_size;
                ry = Random(room_max_size - room_min_size) + room_min_size;
                x2 = x1 + rx;
                y2 = y1 + ry;

                if (x2 >= level.GetWidth() - 1 || y2 >= level.GetHeight() / 2 + 3) {
                    continue;
                }
            }

            // is in existing room?
            bool rand_again = false;

            for (m = list_of_rooms.begin(); m != list_of_rooms.end(); ++m) {
                SRoom &room = *m;
                rand_again = true;

                if (room.IsInRoom(x1, y1)) {
                    if (!room.IsInRoom(x2, y2)) {
                        rand_again = false;
                        room.type++;
                    }

                    break;
                }

                if (room.IsInRoom(x2, y2)) {
                    if (!room.IsInRoom(x1, y1)) {
                        rand_again = false;
                        room.type++;
                    }

                    break;
                }
            }

            if (rand_again) {
                continue;
            }

            // Create room
            number_of_rooms++;
            SRoom new_room;
            new_room.corner1.x = x1;
            new_room.corner1.y = y1;
            new_room.corner2.x = x2;
            new_room.corner2.y = y2;
            new_room.type = number_of_rooms;

            list_of_rooms.push_back(new_room);
        }

        // paint rooms on drafting board
        for (m = list_of_rooms.begin(); m != list_of_rooms.end(); ++m) {
            const SRoom &room = *m;

            for (std::size_t x = room.corner1.x; x <= room.corner2.x; x++) {
                for (std::size_t y = room.corner1.y; y <= room.corner2.y; y++) {
                    if (id_matrix(x, y) == 0) {
                        id_matrix.set(x, y, room.type);
                    }
                }
            }
        }

        std::size_t free_cells = 0;

        //////////////////////////
        // Draw on the real map //
        //////////////////////////

        // fill map with walls
        for (std::size_t x = 0; x < level.GetWidth(); ++x) {
            for (std::size_t y = 0; y < level.GetHeight(); ++y) {
                level.SetCell(x, y, LevelElementWall);
            }
        }

        for (std::size_t x = 0; x < level.GetWidth() - 1; x++) {
            for (std::size_t y = 0; y < level.GetHeight() / 2; y++) {

                int current_id = id_matrix.get(x, y);
                int right_id   = id_matrix.get(x + 1, y);
                int bottom_id  = id_matrix.get(x, y + 1);
                int diag_id    = id_matrix.get(x + 1, y + 1);

                if (current_id != 0) {
                    if ((current_id != right_id  && right_id != 0) ||
                        (current_id != bottom_id && bottom_id != 0) ||
                        (current_id != diag_id   && diag_id != 0))
                    {
                        // Two adjacent rooms - place wall
                        level.SetCell(x, y, LevelElementWall);
                    } else {
                        // No adjacent room, this is the inside of the room
                        level.SetCell(x, y, LevelElementRoom);
                    }
                } else {
                    // solid matter
                    level.SetCell(x, y, LevelElementWall);
                }

                if (level.GetCell(x, y) != LevelElementWall) {
                    free_cells += 2;
                }

                // Mirror the new cell
                level.SetCell(x, level.GetHeight() - y - 1, level.GetCell(x, y));
            }
        }

        // Size of ship
        if (free_cells < level.GetHeight() * level.GetWidth() / 4) {
            continue;
        }

        ConnectClosestRooms(level, true);
        break;
    }

    AddDoors(level, 1, 0);
}

} // end of namespace RL

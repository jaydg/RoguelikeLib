module;

export module rl.mapgenerators.symmetricstructure;

import rl.map;
import rl.randomness;
import rl.maputils;
import rl.matrix;
import rl.position;
import std;

export namespace RL
{

enum class ESymmetryAxis
{
    Horizontal, ///< Mirrored top-to-bottom across y = height / 2 (Space Shuttle)
    Vertical    ///< Mirrored left-to-right across x = width / 2 (Castle)
};

void CreateSymmetricStructure(CMap &level, const int& max_number_of_rooms = 15,
                              ESymmetryAxis axis = ESymmetryAxis::Horizontal)
{
    constexpr int room_max_size = 15;
    constexpr int room_min_size = 3;

    const std::size_t u_max = (axis == ESymmetryAxis::Horizontal) ? level.getWidth() : level.getHeight();
    const std::size_t v_max = (axis == ESymmetryAxis::Horizontal) ? level.getHeight() : level.getWidth();

    int number_of_rooms;

    std::list<SRoom> list_of_rooms;
    std::list<SRoom>::iterator m;

    // create until structure looks good
    while (true) {
        // drafting board in (u, v) space
        CMatrix<int> id_matrix(Size(u_max, v_max), 0);
        list_of_rooms.clear();

        // Create rooms in the half-space v <= v_max / 2
        for (number_of_rooms = 0; number_of_rooms < max_number_of_rooms;) {
            std::size_t u1, v1, u2, v2;
            std::size_t ru, rv;

            if (number_of_rooms == 0) {
                u1 = u_max / 2 - Random(room_max_size);
                v1 = v_max / 2 - Random(room_max_size) - room_min_size;
                ru = Random(room_max_size) + room_min_size;
                rv = Random(room_max_size - room_min_size) + room_min_size;
                u2 = u1 + ru;
                v2 = v_max / 2;

                if (u2 >= u_max) {
                    continue;
                }
            } else {
                u1 = Random(static_cast<int>(u_max) - room_min_size) + 1;
                v1 = Random(static_cast<int>(v_max) - room_min_size) / 2 + 1;
                ru = Random(room_max_size - room_min_size) + room_min_size;
                rv = Random(room_max_size - room_min_size) + room_min_size;
                u2 = u1 + ru;
                v2 = v1 + rv;

                if (u2 >= u_max - 1 || v2 >= v_max / 2 + 3) {
                    continue;
                }
            }

            // is in existing room?
            bool rand_again = false;

            for (m = list_of_rooms.begin(); m != list_of_rooms.end(); ++m) {
                SRoom &room = *m;
                rand_again = true;

                if (room.IsInRoom(Position(u1, v1))) {
                    if (!room.IsInRoom(Position(u2, v2))) {
                        rand_again = false;
                        room.type++;
                    }

                    break;
                }

                if (room.IsInRoom(Position(u2, v2))) {
                    if (!room.IsInRoom(Position(u1, v1))) {
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
            new_room.corner1 = Position(u1, v1);
            new_room.corner2 = Position(u2, v2);
            new_room.type = number_of_rooms;

            list_of_rooms.push_back(new_room);
        }

        // paint rooms on drafting board
        for (m = list_of_rooms.begin(); m != list_of_rooms.end(); ++m) {
            const SRoom &room = *m;

            for (std::size_t u = room.corner1.x; u <= room.corner2.x; u++) {
                for (std::size_t v = room.corner1.y; v <= room.corner2.y; v++) {
                    if (id_matrix.get(u, v) == 0) {
                        id_matrix.set(u, v, room.type);
                    }
                }
            }
        }

        std::size_t free_cells = 0;

        //////////////////////////
        // Draw on the real map //
        //////////////////////////

        // fill map with walls
        for (std::size_t x = 0; x < level.getWidth(); ++x) {
            for (std::size_t y = 0; y < level.getHeight(); ++y) {
                level.SetCell(x, y, "wall");
            }
        }

        for (std::size_t u = 0; u < u_max - 1; u++) {
            for (std::size_t v = 0; v < v_max / 2; v++) {

                int current_id = id_matrix.get(u, v);
                int right_id = id_matrix.get(u + 1, v);
                int bottom_id = id_matrix.get(u, v + 1);
                int diag_id = id_matrix.get(u + 1, v + 1);

                std::string_view tile_type = "wall";
                if (current_id != 0) {
                    if ((current_id != right_id && right_id != 0) ||
                            (current_id != bottom_id && bottom_id != 0) ||
                            (current_id != diag_id && diag_id != 0)) {
                        // Two adjacent rooms - place wall
                        tile_type = "wall";
                    } else {
                        // No adjacent room, this is the inside of the room
                        tile_type = "room";
                    }
                }

                if (tile_type != "wall") {
                    free_cells += 2;
                }

                const Position pos = (axis == ESymmetryAxis::Horizontal) ? Position(u, v) : Position(v, u);
                const Position mirror_pos = (axis == ESymmetryAxis::Horizontal)
                                                ? Position(u, v_max - v - 1)
                                                : Position(v_max - v - 1, u);

                level.SetCell(pos, tile_type);
                level.SetCell(mirror_pos, tile_type);
            }
        }

        // Size of structure
        if (free_cells < level.getHeight() * level.getWidth() / 4) {
            continue;
        }

        ConnectClosestRooms(level, true);
        break;
    }

    AddDoors(level, 1, 0);
}

void CreateSpaceShuttle(CMap &level, const int& max_number_of_rooms = 15)
{
    CreateSymmetricStructure(level, max_number_of_rooms, ESymmetryAxis::Horizontal);
}

void CreateCastle(CMap &level, const int& max_number_of_rooms = 15)
{
    CreateSymmetricStructure(level, max_number_of_rooms, ESymmetryAxis::Vertical);
}

} // end of namespace RL

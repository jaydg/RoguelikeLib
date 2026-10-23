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
                              ESymmetryAxis axis = ESymmetryAxis::Horizontal,
                              bool void_outside = false,
                              bool outside_door = false,
                              float window_probability = 0.0f)
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
                    }

                    break;
                }

                if (room.IsInRoom(Position(u2, v2))) {
                    if (!room.IsInRoom(Position(u1, v1))) {
                        rand_again = false;
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

    if (void_outside) {
        std::vector<Position> outside_walls;
        outside_walls.reserve(level.getWidth() * level.getHeight());
        for (std::size_t y = 0; y < level.getHeight(); ++y) {
            for (std::size_t x = 0; x < level.getWidth(); ++x) {
                if (level.get(x, y).getType() == "wall") {
                    bool has_interior = false;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) {
                                continue;
                            }
                            const int nx = static_cast<int>(x) + dx;
                            const int ny = static_cast<int>(y) + dy;
                            if (nx >= 0 && nx < static_cast<int>(level.getWidth()) &&
                                    ny >= 0 && ny < static_cast<int>(level.getHeight())) {
                                const auto type = level.get(nx, ny).getType();
                                if (type != "wall" && type != "void") {
                                    has_interior = true;
                                    break;
                                }
                            }
                        }
                        if (has_interior) {
                            break;
                        }
                    }
                    if (!has_interior) {
                        outside_walls.push_back(Position(x, y));
                    }
                }
            }
        }
        for (const auto& pos : outside_walls) {
            level.SetCell(pos, "void");
        }
    }

    if (outside_door) {
        const int w = static_cast<int>(level.getWidth());
        const int h = static_cast<int>(level.getHeight());

        if (axis == ESymmetryAxis::Vertical) {
            const int center_x = w / 2;
            const bool is_even = (w % 2 == 0);

            auto is_interior = [&](const auto& tile) -> bool {
                return tile.getType() != "wall" && tile.getType() != "void";
            };

            auto try_place = [&](int x_left, int x_right) -> bool {
                for (int y = h - 2; y >= 1; --y) {
                    const auto t_left = level.get(x_left, y).getType();
                    const auto t_right = level.get(x_right, y).getType();
                    if (t_left == "wall" && t_right == "wall") {
                        const auto south_left = (y + 1 < h) ? level.get(x_left, y + 1).getType() : "void";
                        const auto south_right = (y + 1 < h) ? level.get(x_right, y + 1).getType() : "void";
                        if (south_left != "void" && south_right != "void") {
                            continue;
                        }

                        int y_in = -1;
                        for (int ny = y - 1; ny >= std::max(1, y - 4); --ny) {
                            if (is_interior(level.get(x_left, ny)) && is_interior(level.get(x_right, ny))) {
                                y_in = ny;
                                break;
                            }
                        }
                        if (y_in != -1) {
                            for (int k = y_in + 1; k < y; ++k) {
                                level.SetCell(x_left, k, "corridor");
                                level.SetCell(x_right, k, "corridor");
                            }
                            level.SetCell(x_left, y, "door_closed", U'-');
                            level.SetCell(x_right, y, "door_closed", U'-');
                            return true;
                        }
                    }
                }
                return false;
            };

            bool placed = is_even ? try_place(center_x - 1, center_x) : try_place(center_x, center_x);
            if (!placed) {
                for (int offset = 1; offset < w / 2; ++offset) {
                    const int xl = is_even ? (center_x - 1 - offset) : (center_x - offset);
                    const int xr = center_x + offset;
                    if (xl >= 0 && xr < w && try_place(xl, xr)) {
                        break;
                    }
                }
            }
        } else {
            const int center_y = h / 2;
            const bool is_even = (h % 2 == 0);

            auto is_interior_h = [&](const auto& tile) -> bool {
                return tile.getType() != "wall" && tile.getType() != "void";
            };

            auto try_place_h = [&](int y_top, int y_bottom) -> bool {
                for (int x = w - 2; x >= 1; --x) {
                    const auto t_top = level.get(x, y_top).getType();
                    const auto t_bottom = level.get(x, y_bottom).getType();
                    if (t_top == "wall" && t_bottom == "wall") {
                        const auto east_top = (x + 1 < w) ? level.get(x + 1, y_top).getType() : "void";
                        const auto east_bottom = (x + 1 < w) ? level.get(x + 1, y_bottom).getType() : "void";
                        if (east_top != "void" && east_bottom != "void") {
                            continue;
                        }

                        int x_in = -1;
                        for (int nx = x - 1; nx >= std::max(1, x - 4); --nx) {
                            if (is_interior_h(level.get(nx, y_top)) && is_interior_h(level.get(nx, y_bottom))) {
                                x_in = nx;
                                break;
                            }
                        }
                        if (x_in != -1) {
                            for (int k = x_in + 1; k < x; ++k) {
                                level.SetCell(k, y_top, "corridor");
                                level.SetCell(k, y_bottom, "corridor");
                            }
                            level.SetCell(x, y_top, "door_closed", U'|');
                            level.SetCell(x, y_bottom, "door_closed", U'|');
                            return true;
                        }
                    }
                }
                return false;
            };

            bool placed = is_even ? try_place_h(center_y - 1, center_y) : try_place_h(center_y, center_y);
            if (!placed) {
                for (int offset = 1; offset < h / 2; ++offset) {
                    const int yt = is_even ? (center_y - 1 - offset) : (center_y - offset);
                    const int yb = center_y + offset;
                    if (yt >= 0 && yb < h && try_place_h(yt, yb)) {
                        break;
                    }
                }
            }
        }
    }

    if (window_probability > 0.0f) {
        const int w = static_cast<int>(level.getWidth());
        const int h = static_cast<int>(level.getHeight());

        CMatrix<bool> exterior_filler(Size(w, h), false);
        if (!void_outside) {
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (level.get(x, y).getType() == "wall") {
                        bool has_interior = false;
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                if (dx == 0 && dy == 0) continue;
                                const int nx = x + dx;
                                const int ny = y + dy;
                                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                    const auto type = level.get(nx, ny).getType();
                                    if (type != "wall" && type != "void") {
                                        has_interior = true;
                                        break;
                                    }
                                }
                            }
                            if (has_interior) break;
                        }
                        if (!has_interior) {
                            exterior_filler.set(x, y, true);
                        }
                    }
                }
            }
        }

        auto is_interior = [&](int nx, int ny) -> bool {
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) return false;
            const auto t = level.get(nx, ny).getType();
            return t == "room" || t == "corridor";
        };

        auto is_exterior = [&](int nx, int ny) -> bool {
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) return true;
            const auto t = level.get(nx, ny).getType();
            if (t == "void") return true;
            if (!void_outside && exterior_filler.get(nx, ny)) return true;
            return false;
        };

        auto get_mirror = [&](int x, int y) -> Position {
            if (axis == ESymmetryAxis::Vertical) {
                return Position(static_cast<std::size_t>(w - 1 - x), static_cast<std::size_t>(y));
            } else {
                return Position(static_cast<std::size_t>(x), static_cast<std::size_t>(h - 1 - y));
            }
        };

        auto near_door = [&](int px, int py) -> bool {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = px + dx;
                    const int ny = py + dy;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        const auto t = level.get(nx, ny).getType();
                        if (t == "door_closed" || t == "door_open") {
                            return true;
                        }
                    }
                }
            }
            return false;
        };

        auto near_window = [&](int px, int py) -> bool {
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    const int nx = px + dx;
                    const int ny = py + dy;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        if (level.get(nx, ny).getType() == "window") {
                            return true;
                        }
                    }
                }
            }
            return false;
        };

        auto get_outside_wall_glyph = [&](int x, int y) -> char32_t {
            if (level.get(x, y).getType() != "wall") return 0;

            // Horizontal wall candidate:
            if (x > 0 && x + 1 < w &&
                level.get(x - 1, y).getType() == "wall" &&
                level.get(x + 1, y).getType() == "wall") {
                const bool north_int = is_interior(x, y - 1);
                const bool south_int = is_interior(x, y + 1);
                const bool north_ext = is_exterior(x, y - 1);
                const bool south_ext = is_exterior(x, y + 1);
                if ((north_int && south_ext) || (south_int && north_ext)) {
                    return U'-';
                }
            }

            // Vertical wall candidate:
            if (y > 0 && y + 1 < h &&
                level.get(x, y - 1).getType() == "wall" &&
                level.get(x, y + 1).getType() == "wall") {
                const bool west_int = is_interior(x - 1, y);
                const bool east_int = is_interior(x + 1, y);
                const bool west_ext = is_exterior(x - 1, y);
                const bool east_ext = is_exterior(x + 1, y);
                if ((west_int && east_ext) || (east_int && west_ext)) {
                    return U'|';
                }
            }

            return 0;
        };

        const int max_x = (axis == ESymmetryAxis::Vertical) ? (w / 2) : (w - 1);
        const int max_y = (axis == ESymmetryAxis::Horizontal) ? (h / 2) : (h - 1);

        for (int y = 0; y <= max_y; ++y) {
            for (int x = 0; x <= max_x; ++x) {
                const char32_t g = get_outside_wall_glyph(x, y);
                if (g == 0) continue;

                const Position mirror_p = get_mirror(x, y);
                const int mx = static_cast<int>(mirror_p.x);
                const int my = static_cast<int>(mirror_p.y);

                const char32_t mg = get_outside_wall_glyph(mx, my);
                if (mg == 0) continue;

                if (near_door(x, y) || near_door(mx, my)) continue;
                if (near_window(x, y) || near_window(mx, my)) continue;

                if (x != mx || y != my) {
                    if (std::max(std::abs(x - mx), std::abs(y - my)) <= 2) {
                        continue;
                    }
                }

                if (window_probability < 1.0f) {
                    const float roll = static_cast<float>(Random(1000)) / 1000.0f;
                    if (roll >= window_probability) {
                        continue;
                    }
                }

                level.SetCell(x, y, "window", g);
                level.SetCell(mx, my, "window", mg);
            }
        }
    }

    UpdateDoorAndWindowGlyphs(level);
}

void CreateSpaceShuttle(CMap &level, const int& max_number_of_rooms = 15, bool void_outside = false, bool outside_door = false, float window_probability = 0.0f)
{
    CreateSymmetricStructure(level, max_number_of_rooms, ESymmetryAxis::Horizontal, void_outside, outside_door, window_probability);
}

void CreateCastle(CMap &level, const int& max_number_of_rooms = 15, bool void_outside = false, bool outside_door = false, float window_probability = 0.0f)
{
    CreateSymmetricStructure(level, max_number_of_rooms, ESymmetryAxis::Vertical, void_outside, outside_door, window_probability);
}

inline void CreateSymmetricStructure(CMap &level, const int& max_number_of_rooms, ESymmetryAxis axis, bool void_outside, bool outside_door, bool windows)
{
    CreateSymmetricStructure(level, max_number_of_rooms, axis, void_outside, outside_door, windows ? 0.30f : 0.0f);
}

inline void CreateSpaceShuttle(CMap &level, const int& max_number_of_rooms, bool void_outside, bool outside_door, bool windows)
{
    CreateSpaceShuttle(level, max_number_of_rooms, void_outside, outside_door, windows ? 0.30f : 0.0f);
}

inline void CreateCastle(CMap &level, const int& max_number_of_rooms, bool void_outside, bool outside_door, bool windows)
{
    CreateCastle(level, max_number_of_rooms, void_outside, outside_door, windows ? 0.30f : 0.0f);
}

} // end of namespace RL

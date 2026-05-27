//////////////////////////////////////////////////////////////////////////
// Map generation helpers
//////////////////////////////////////////////////////////////////////////

module;

export module rl.maputils;

import rl.distance;
import rl.map;
import rl.matrix;
import rl.position;
import rl.randomness;
import std;

namespace RL {

// Internal Math and Path Helpers
namespace detail {
    int Sqr(int x) {
        return x * x;
    }

    void CutCorners(std::vector<Position>& seq) {
        if (seq.size() < 3) return;

        std::size_t j = 1;
        for (std::size_t i = 1; i < seq.size() - 1; ++i) {
            seq[j] = seq[i];
            int dx = std::abs(static_cast<int>(seq[j - 1].x - seq[i + 1].x));
            int dy = std::abs(static_cast<int>(seq[j - 1].y - seq[i + 1].y));
            if (std::max(dx, dy) > 1) {
                j++;
            }
        }
        seq[j] = seq.back();
        j++;
        seq.resize(j);
    }

    void BuildZigzagPath(std::vector<Position>& ret, const Position& p1, const Position& p2, unsigned turnpct, unsigned diagpct) {
        ret.clear();
        int xc = static_cast<int>(p1.x);
        int yc = static_cast<int>(p1.y);
        int x2 = static_cast<int>(p2.x);
        int y2 = static_cast<int>(p2.y);

        int deltax = 0, deltay = 0;
        ret.emplace_back(xc, yc);

        while (xc != x2 || yc != y2) {
            int xremain = std::abs(x2 - xc);
            int yremain = std::abs(y2 - yc);

            if (ret.size() == 1 || (Random(100) < turnpct) ||
                (std::abs(x2 - (xc + deltax)) > xremain) ||
                (std::abs(y2 - (yc + deltay)) > yremain) ||
                ((xremain == yremain) && (Random(100) < diagpct))) {

                deltax = Sign(x2 - xc);
                deltay = Sign(y2 - yc);

                if (Random(100) < diagpct) {
                    if (xremain > yremain) {
                        if (static_cast<int>(Random(xremain)) < (xremain - yremain)) {
                            deltay = 0;
                        }
                    } else if (xremain < yremain) {
                        if (static_cast<int>(Random(yremain)) < (yremain - xremain)) {
                            deltax = 0;
                        }
                    }
                } else {
                    if (static_cast<int>(Random(xremain + yremain)) < xremain) {
                        if (deltax != 0) {
                            deltay = 0;
                        }
                    } else {
                        if (deltay != 0) {
                            deltax = 0;
                        }
                    }
                }
            }

            xc += deltax;
            yc += deltay;
            ret.emplace_back(xc, yc);
        }
    }

    int SignCos2(const Position& p0, const Position& p1, const Position& p2) {
        int sqlen01 = Sqr(static_cast<int>(p1.x - p0.x)) + Sqr(static_cast<int>(p1.y - p0.y));
        int sqlen12 = Sqr(static_cast<int>(p2.x - p1.x)) + Sqr(static_cast<int>(p2.y - p1.y));
        if (sqlen01 == 0 || sqlen12 == 0) return 0;

        int prod = static_cast<int>((p1.x - p0.x) * (p2.x - p1.x) + (p1.y - p0.y) * (p2.y - p1.y));
        long long prod_ll = prod; // Prevent overflow
        long long val = 1000LL * (prod_ll * prod_ll / sqlen01) / sqlen12;
        if (prod < 0) {
            val = -val;
        }
        return static_cast<int>(val);
    }

    void PerturbPath(std::vector<Position>& way, const CMap& level, int mindist, int maxdist, int pertamt) {
        if (way.size() < 3) return;

        static const int Xoff[8] = {1,  1,  0, -1, -1, -1,  0,  1};
        static const int Yoff[8] = {0,  1,  1,  1,  0, -1, -1, -1};
        const int mincos2 = 500;

        int mind2 = Sqr(mindist);
        int maxd2 = Sqr(maxdist);

        std::size_t loops = static_cast<std::size_t>(pertamt) * way.size();
        for (std::size_t i = 0; i < loops; ++i) {
            std::size_t ri = 1 + Random(static_cast<int>(way.size()) - 2);
            int rdir = Random(8);
            int nx = static_cast<int>(way[ri].x) + Xoff[rdir];
            int ny = static_cast<int>(way[ri].y) + Yoff[rdir];

            if (nx < 1 || nx >= static_cast<int>(level.GetWidth()) - 1 ||
                ny < 1 || ny >= static_cast<int>(level.GetHeight()) - 1) {
                continue;
            }

            int lox = static_cast<int>(way[ri - 1].x);
            int loy = static_cast<int>(way[ri - 1].y);
            int hix = static_cast<int>(way[ri + 1].x);
            int hiy = static_cast<int>(way[ri + 1].y);

            int lod2 = Sqr(nx - lox) + Sqr(ny - loy);
            int hid2 = Sqr(nx - hix) + Sqr(ny - hiy);

            if (lod2 < mind2 || lod2 > maxd2 || hid2 < mind2 || hid2 > maxd2) {
                continue;
            }

            if (SignCos2(Position(lox, loy), Position(nx, ny), Position(hix, hiy)) < mincos2) {
                continue;
            }

            if (ri > 1 && SignCos2(way[ri - 2], Position(lox, loy), Position(nx, ny)) < mincos2) {
                continue;
            }

            if (ri < way.size() - 2 && SignCos2(Position(nx, ny), Position(hix, hiy), way[ri + 2]) < mincos2) {
                continue;
            }

            way[ri] = Position(nx, ny);
        }
    }

    void ConnectWaypoints(std::vector<Position>& result, const std::vector<Position>& waypts) {
        result.clear();
        if (waypts.size() <= 1) return;

        result.push_back(waypts[0]);

        for (std::size_t i = 0; i < waypts.size() - 1; ++i) {
            std::vector<Position> segment = waypts[i].BuildBresenhamLine(waypts[i + 1]);
            for (std::size_t j = 1; j < segment.size(); ++j) {
                result.push_back(segment[j]);
            }
        }
    }
} // end of detail

} // end of RL namespace (internal)

// Public API (exported)
export namespace RL {

void FindOnMapAllRectanglesOfType(CMap &level, const std::string_view type, const Size &size, std::vector <Position>& positions)
{
    auto good_points = CMatrix<bool>(level.getSize(), false);

    for (std::size_t y = 0; y < level.GetHeight(); ++y) {
        std::size_t horizontal_count = 0;

        for (std::size_t x = 0; x < level.GetWidth(); ++x) {
            if (level.GetCell(x, y).getType() == type) {
                horizontal_count++;
            } else {
                horizontal_count = 0;
            }

            if (horizontal_count == size.x) {
                good_points.set(x - size.x + 1, y, 1);
                horizontal_count--;
            }
        }
    }

    // count verticals
    for (std::size_t x = 0; x < level.GetWidth(); ++x) {
        std::size_t vertical_count = 0;

        for (std::size_t y = 0; y < level.GetHeight(); ++y) {
            if (good_points(x, y)) {
                vertical_count++;
            } else {
                vertical_count = 0;
            }

            if (vertical_count == size.y) {
                positions.emplace_back(x, y - size.y + 1);
                vertical_count--;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

bool FindOnMapRandomRectangleOfType(CMap &level, std::string_view type, Position& pos, const Size &size)
{
    std::vector<Position> positions;
    FindOnMapAllRectanglesOfType(level, type, size, positions);

    if(positions.empty()) {
        return false;
    }

    // get position of Random rectangle
    pos = positions[Random(static_cast<std::size_t>(positions.size()))];
    return true;
}

//////////////////////////////////////////////////////////////////////////

int CountNeighboursOfType(CMap &level, std::string_view type, const Position& pos, bool diagonal = true)
{
    int neighbours = 0;

    if(pos.y > 0)
        if(level.GetCell(pos.x, pos.y - 1).getType() == type) { // N
            neighbours++;
        }

    if(pos.x < level.GetWidth() - 1)
        if(level.GetCell(pos.x + 1, pos.y).getType() == type) { // E
            neighbours++;
        }

    if(pos.x > 0 && pos.y < level.GetHeight() - 1)
        if(level.GetCell(pos.x, pos.y + 1).getType() == type) { // S
            neighbours++;
        }

    if(pos.x > 0 && pos.y > 0)
        if(level.GetCell(pos.x - 1, pos.y).getType() == type) { // W
            neighbours++;
        }

    if(diagonal) {
        if(pos.x > 0 && pos.y > 0)
            if(level.GetCell(pos.x - 1, pos.y - 1).getType() == type) { // NW
                neighbours++;
            }

        if(pos.x < level.GetWidth() - 1 && pos.y > 0)
            if(level.GetCell(pos.x + 1, pos.y - 1).getType() == type) { // NE
                neighbours++;
            }

        if(pos.x < level.GetWidth() - 1 && pos.y < level.GetHeight() - 1) // SE
            if(level.GetCell(pos.x + 1, pos.y + 1).getType() == type) {
                neighbours++;
            }


        if(pos.x > 0 && pos.y < level.GetHeight() - 1)
            if(level.GetCell(pos.x - 1, pos.y + 1).getType() == type) { // SW
                neighbours++;
            }
    }

    return neighbours;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void AddDoors(CMap &level, float door_probability, float open_probability)
{
    for(std::size_t x = 0; x < level.GetWidth(); ++x) {
        for(std::size_t y = 0; y < level.GetHeight(); ++y) {
            Position pos(x, y);
            int room_cells = CountNeighboursOfType(level, "room", pos);
            int corridor_cells = CountNeighboursOfType(level, "corridor", pos);
            int open_door_cells = CountNeighboursOfType(level, "door_open", pos);
            int close_door_cells = CountNeighboursOfType(level, "door_closed", pos);
            int door_cells = open_door_cells + close_door_cells;

            if(level.GetCell(x, y).getType() == "corridor") {
                if((corridor_cells == 1 && door_cells == 0 && room_cells > 0 && room_cells < 4) ||
                        (corridor_cells == 0 && door_cells == 0)) {
                    float exist = (static_cast<float>(Random(1000))) / 1000.0f;

                    if(exist < door_probability) {
                        float is_open = (static_cast<float>(Random(1000))) / 1000.0f;

                        if(is_open < open_probability) {
                            level.SetCell(x, y, "door_open");
                        } else {
                            level.SetCell(x, y, "door_closed");
                        }
                    }
                }
            } // if corridor at position
        } // for y < level height
    } // for x < level width
}

//////////////////////////////////////////////////////////////////////////

bool AddCorridor(CMap &level, const std::size_t &start_x1, const std::size_t &start_y1, const std::size_t &start_x2, const std::size_t &start_y2, bool straight = false)
{
    if(!level.OnMap(start_x1, start_y1) || !level.OnMap(start_x2, start_y2)) {
        return false;
    }

    // we start from both sides
    std::size_t x1 = start_x1;
    std::size_t y1 = start_y1;
    std::size_t x2 = start_x2;
    std::size_t y2 = start_y2;

    int dir_x;
    int dir_y;

    if(start_x2 > start_x1) {
        dir_x = 1;
    } else {
        dir_x = -1;
    }

    if(start_y2 > start_y1) {
        dir_y = 1;
    } else {
        dir_y = -1;
    }

    // move into direction of the other end
    bool first_horizontal = CoinToss();
    bool second_horizontal = CoinToss();

    while(true) {
        if(!straight) {
            first_horizontal = CoinToss();
            second_horizontal = CoinToss();
        }

        if(x1 != x2 && y1 != y2) {
            if(first_horizontal) {
                x1 += dir_x;
            } else {
                y1 += dir_y;
            }
        }

        // connect rooms
        if(x1 != x2 && y1 != y2) {
            if(second_horizontal) {
                x2 -= dir_x;
            } else {
                y2 -= dir_y;
            }
        }

        if(!level.GetCell(x1, y1).isPassable()) {
            level.SetCell(x1, y1, "corridor");
        }

        if(!level.GetCell(x2, y2).isPassable()) {
            level.SetCell(x2, y2, "corridor");
        }

        // connect corridors if on the same level
        if (x1 == x2) {
            while (y1 != y2) {
                y1 += dir_y;

                if(!level.GetCell(x1, y1).isPassable()) {
                    level.SetCell(x1, y1, "corridor");
                }
            }

            if(!level.GetCell(x1, y1) .isPassable()) {
                level.SetCell(x1, y1, "corridor");
            }

            return true;
        }

        if (y1 == y2) {
            while (x1 != x2) {
                x1 += dir_x;

                if(!level.GetCell(x1, y1).isPassable()) {
                    level.SetCell(x1, y1, "corridor");
                }
            }

            if(!level.GetCell(x1, y1).isPassable()) {
                level.SetCell(x1, y1, "corridor");
            }

            return true;
        }
    } // while (true)
}

//////////////////////////////////////////////////////////////////////////

CMatrix<int> FillDisconnectedRoomsWithDifferentValues(const CMap &level) {
    constexpr int any_room = -1;

    auto flood_map = CMatrix<int>(level.getSize(), 0);

    for (std::size_t y = 0; y < level.GetHeight(); ++y) {
        for (std::size_t x = 0; x < level.GetWidth(); ++x) {
            if (level.GetCell(x, y).getType() == "room") {
                flood_map.set(x, y, any_room);
            }
        }
    }

    int room_number = 1;

    for (std::size_t y = 0; y < level.GetHeight(); ++y) {
        for (std::size_t x = 0; x < level.GetWidth(); ++x) {
            if (flood_map(x, y) == any_room) {
                flood_map.FloodFill(Position(x, y), room_number++);
            }
        }
    }

    return flood_map;
}

//////////////////////////////////////////////////////////////////////////

// TODO: with_doors is not implemented, as doors are not implemented
void ConnectClosestRooms(CMap &level, [[maybe_unused]] bool with_doors, bool straight_connections = false)
{
    // fill each room on the map with an individual number
    auto floodmap = FillDisconnectedRoomsWithDifferentValues(level);

    // This vector keeps a list of border cells that are not at a corner
    // for each room. These cells are candidates as starting points for
    // corridors to the other rooms.
    std::vector<std::list<Position>> rooms;

    // build the vector of rooms
    for (std::size_t y = 0; y < floodmap.getHeight(); ++y) {
        for (std::size_t x = 0; x < floodmap.getWidth(); ++x) {
            if (floodmap(x, y) > 0) {
                // grow room list when required
                if (floodmap(x, y) >= static_cast<int>(rooms.size())) {
                    rooms.resize(floodmap(x, y) + 1);
                }

                // only border cells without diagonals
                if (floodmap.CountNeighbors(Position(x, y),0, Neighbors::Cardinal4) > 0)
                {
                    rooms[floodmap(x, y)].emplace_back(x, y);
                }
            } // if no wall at position
        } // for x
    } // for y

    if(rooms.size() < 2) {
        return;
    }

    Shuffle(rooms.begin(), rooms.end());

    // for warshall algorithm
    // set the connection matrix

    std::vector<std::vector<bool>> room_connections;
    std::vector<std::vector<bool>> transitive_closure;
    std::vector<std::vector<std::size_t>> distance_matrix;
    std::vector<std::vector<std::pair<Position, Position>>> closest_cells_matrix;

    room_connections.resize(rooms.size());
    transitive_closure.resize(rooms.size());
    distance_matrix.resize(rooms.size());
    closest_cells_matrix.resize(rooms.size());

    for (std::size_t a = 0; a < rooms.size(); ++a) {
        room_connections[a].resize(rooms.size());
        transitive_closure[a].resize(rooms.size());
        distance_matrix[a].resize(rooms.size());
        closest_cells_matrix[a].resize(rooms.size());

        for (std::size_t b = 0; b < rooms.size(); ++b) {
            room_connections[a][b] = false;
            distance_matrix[a][b] = std::numeric_limits<int>::max();
        }
    }

    // find the closest cells for each room - random closest cell
    std::list<Position>::iterator m, _m, k, _k;

    for (std::size_t room_a = 0; room_a < rooms.size(); ++room_a) {
        for (std::size_t room_b = 0; room_b < rooms.size(); ++room_b) {
            if (room_a == room_b) {
                continue;
            }

            std::pair<Position, Position> closest_cells;

            for (m = rooms[room_a].begin(), _m = rooms[room_a].end(); m != _m; ++m) {
                // for each border cell in room_a try each border cell of room_b
                std::size_t x1 = m->x;
                std::size_t y1 = m->y;

                for (k = rooms[room_b].begin(), _k = rooms[room_b].end(); k != _k; ++k) {
                    std::size_t x2 = k->x;
                    std::size_t y2 = k->y;

                    std::size_t dist_ab = Distance(x1, y1, x2, y2);

                    if (dist_ab < distance_matrix[room_a][room_b] || (dist_ab == distance_matrix[room_a][room_b] && CoinToss())) {
                        closest_cells = std::make_pair(Position(x1, y1), Position(x2, y2));
                        distance_matrix[room_a][room_b] = dist_ab;
                    }
                }
            }

            closest_cells_matrix[room_a][room_b] = closest_cells;
        }
    }

    // Now connect the rooms to the closest ones
    for (std::size_t room_a = 0; room_a < rooms.size(); ++room_a) {
        std::size_t min_distance = std::numeric_limits<std::size_t>::max();
        std::size_t closest_room = 0;

        for (std::size_t room_b = 0; room_b < rooms.size(); ++room_b) {
            if (room_a == room_b) {
                continue;
            }

            std::size_t distance = distance_matrix[room_a][room_b];

            if(distance < min_distance) {
                min_distance = distance;
                closest_room = room_b;
            }
        }

        // connect room_a to closest one
        std::pair<Position, Position> closest_cells;
        closest_cells = closest_cells_matrix[room_a][closest_room];

        std::size_t x1 = closest_cells.first.x;
        std::size_t y1 = closest_cells.first.y;
        std::size_t x2 = closest_cells.second.x;
        std::size_t y2 = closest_cells.second.y;

        if (!room_connections[room_a][closest_room] && AddCorridor(level, x1, y1, x2, y2, straight_connections)) {
            room_connections[room_a][closest_room] = true;
            room_connections[closest_room][room_a] = true;
        }
    }

    // The closest rooms connected. Connect the rest until all areas are connected
    for (int to_connect_a = 0; to_connect_a != -1;) {
        std::size_t a, b, c;

        for (a = 0; a < rooms.size(); a++) {
            for (b = 0; b < rooms.size(); b++) {
                transitive_closure[a][b] = room_connections[a][b];
            }
        }

        for (a = 0; a < rooms.size(); a++) {
            for (b = 0; b < rooms.size(); b++) {
                if (transitive_closure[a][b] && a != b) {
                    for (c = 0; c < rooms.size(); c++) {
                        if (transitive_closure[b][c]) {
                            transitive_closure[a][c] = true;
                            transitive_closure[c][a] = true;
                        }
                    }
                }
            }
        }

        // Check if all rooms are connected
        to_connect_a = -1;

        for (a = 0; a < rooms.size() && to_connect_a == -1; ++a) {
            for (b = 0; b < rooms.size(); b++) {
                if (a != b && !transitive_closure[a][b]) {
                    to_connect_a = static_cast<int>(a);
                    break;
                }
            }
        }

        if (to_connect_a != -1) {
            int to_connect_b;

            // connect rooms a & b
            do {
                to_connect_b = static_cast<int>(Random(rooms.size()));
            } while (to_connect_b == to_connect_a);

            std::pair < Position, Position > closest_cells;
            closest_cells = closest_cells_matrix[to_connect_a][to_connect_b];

            std::size_t x1 = closest_cells.first.x;
            std::size_t y1 = closest_cells.first.y;
            std::size_t x2 = closest_cells.second.x;
            std::size_t y2 = closest_cells.second.y;

            AddCorridor(level, x1, y1, x2, y2, straight_connections);

            room_connections[to_connect_a][to_connect_b] = true;
            room_connections[to_connect_b][to_connect_a] = true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void AddRecursiveRooms(CMap &level, std::string_view type, std::size_t min_size_x, std::size_t min_size_y, const SRoom& room, bool with_doors = true)
{
    std::size_t size_x = room.corner2.x - room.corner1.x;

    if(size_x % 2 != 0) {
        size_x -= CoinToss();
    }

    std::size_t size_y = room.corner2.y - room.corner1.y;

    if(size_y % 2 != 0) {
        size_y -= CoinToss();
    }

    bool split_horizontal;

    if(size_y * 4 > size_x) {
        split_horizontal = true;
    } else if(size_x * 4 > size_y) {
        split_horizontal = false;
    } else {
        split_horizontal = CoinToss();
    }

    if(split_horizontal) { // split horizontal
        if(size_y / 2 < min_size_y) {
            return;
        }

        const std::size_t split = size_y / 2 + Random(size_y / 2 - min_size_y);

        for(std::size_t x = room.corner1.x; x < room.corner2.x; x++) {
            level.SetCell(x, room.corner1.y + split, type);
        }

        if(with_doors) {
            level.SetCell(room.corner1.x + Random(size_x - 1) + 1, room.corner1.y + split, "door_closed");
        }

        SRoom new_room = room;
        new_room.corner2.y = room.corner1.y + split;
        AddRecursiveRooms(level, type, min_size_x, min_size_y, new_room, with_doors);

        new_room = room;
        new_room.corner1.y = room.corner1.y + split;
        AddRecursiveRooms(level, type, min_size_x, min_size_y, new_room, with_doors);
    } else {
        if(size_x / 2 < min_size_x) {
            return;
        }

        const std::size_t split = size_x / 2 + Random(size_x / 2 - min_size_x);

        for(std::size_t y = room.corner1.y; y < room.corner2.y; y++) {
            level.SetCell(room.corner1.x + split, y, type);
        }

        if(with_doors) {
            level.SetCell(
                room.corner1.x + split,
                room.corner1.y + Random(size_y - 1) + 1,
                "door_closed");
        }

        SRoom new_room = room;
        new_room.corner2.x = room.corner1.x + split;
        AddRecursiveRooms(level, type, min_size_x, min_size_y, new_room, with_doors);

        new_room = room;
        new_room.corner1.x = room.corner1.x + split;
        AddRecursiveRooms(level, type, min_size_x, min_size_y, new_room, with_doors);
    }
}

//////////////////////////////////////////////////////////////////////////

void DrawRectangleOnMap(CMap &level, const Position& p1, const Position& p2, std::string_view value)
{
    for(std::size_t y = p1.y; y < p2.y; ++y) {
        for(std::size_t x = p1.x; x < p2.x; ++x) {
            level.SetCell(x, y, value);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// Refactored road algorithms from windroad.c
//////////////////////////////////////////////////////////////////////////

/* Written by Kusigrosz in September 2010 (parts much earlier)
 * This program is in public domain, with all its bugs etc.
 * No warranty whatsoever.
 *
 * Generating winding roads/corridors for a roguelike game.
 *
 * The functions that do road generating are:
 *
 * uti_windroad(&road, mpc, x1, y1, x2, y2, pertamt)
 *     Generates a winding road from x1, y1 to x2, y2 without
 *     sharp turns. The road winds regardless of the relative location
 *     of endpoints (unless it is too short). The parameter pertamt
 *     controls the degree of perturbation the initially straight road
 *     is subjected to; typical values of 5-50 give decent results.
 *     mpc is the pointer to the map structure (needed to make sure
 *     the winding road stays within the map.
 *
 * uti_zigzag(&road, x1, y1, x2, y2, turnpct, diagpct)
 *     Generates a randomly zigzagging road from x1, y1 to x2, y2
 *     The road zigzags only if the endpoints differ in both coordinates,
 *     Otherwise it is a straight line. The parameter turnpct is the
 *     chance of a non-forced turn in percent (so, 100/turnpct is
 *     approximately the length of a straight segment; diagpct is
 *     the chance that a diagonal turn is allowed.
 *
 * uti_sigsag(&road, x1, y1, x2, y2, turnpct, diagpct)
 *     The same as zigzag, but without sharp corners.
 */


bool AddWindingCorridor(CMap &level, const Position& start, const Position& end, int pertamt) {
    if (!level.OnMap(start.x, start.y) || !level.OnMap(end.x, end.y)) {
        return false;
    }

    std::vector<Position> waypts = start.BuildBresenhamLine(end);

    std::vector<Position> road;
    if (waypts.size() < 5) {
        road = waypts;
    } else {
        std::vector<Position> sampled_waypts;
        for (std::size_t i = 0; i < waypts.size(); ) {
            sampled_waypts.push_back(waypts[i]);
            if (i < waypts.size() - 5) {
                i += 2 + Random(2);
            } else if (i == waypts.size() - 5) {
                i += 2;
            } else {
                i = waypts.size() - 1;
            }
        }
        waypts = sampled_waypts;

        detail::PerturbPath(waypts, level, 2, 5, pertamt);
        detail::ConnectWaypoints(road, waypts);
        detail::CutCorners(road);
    }

    for (const auto& pos : road) {
        if (pos.x >= 1 && pos.x < level.GetWidth() - 1 &&
            pos.y >= 1 && pos.y < level.GetHeight() - 1) {
            level.SetCell(pos.x, pos.y, "corridor");
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////

bool AddZigzagCorridor(CMap &level, const Position& start, const Position& end, int turnpct, int diagpct) {
    if (!level.OnMap(start.x, start.y) || !level.OnMap(end.x, end.y)) {
        return false;
    }

    std::vector<Position> road;
    detail::BuildZigzagPath(road, start, end, turnpct, diagpct);

    for (const auto& pos : road) {
        if (pos.x >= 1 && pos.x < level.GetWidth() - 1 &&
            pos.y >= 1 && pos.y < level.GetHeight() - 1) {
            level.SetCell(pos.x, pos.y, "corridor");
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////

bool AddSigsagCorridor(CMap &level, const Position& start, const Position& end, int turnpct, int diagpct) {
    if (!level.OnMap(start.x, start.y) || !level.OnMap(end.x, end.y)) {
        return false;
    }

    std::vector<Position> road;
    detail::BuildZigzagPath(road, start, end, turnpct, diagpct);
    detail::CutCorners(road);

    for (const auto& pos : road) {
        if (pos.x >= 1 && pos.x < level.GetWidth() - 1 &&
            pos.y >= 1 && pos.y < level.GetHeight() - 1) {
            level.SetCell(pos.x, pos.y, "corridor");
        }
    }
    return true;
}

} // end of export namespace RL

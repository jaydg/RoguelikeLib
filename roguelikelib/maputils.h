//////////////////////////////////////////////////////////////////////////
// Map generation helpers
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_MAPUTILS_H
#define RL_MAPUTILS_H

#include "map.h"
#include "randomness.h"
#include "distance.h"

#include <algorithm>
#include <climits>
#include <list>

namespace RL {
inline
void FindOnMapAllRectanglesOfType(CMap &level, const RL::ELevelElement& type, const Size &size, std::vector < RL::Position >& positions)
{
    CMap good_points;
    good_points = level;

    for(unsigned int y = 0; y < level.GetHeight(); ++y)
        for(unsigned int x = 0; x < level.GetWidth(); ++x) {
            good_points.SetCell(x, y, 0);
        }

    for(unsigned int y = 0; y < level.GetHeight(); ++y) {
        int horizontal_count = 0;

        for(unsigned int x = 0; x < level.GetWidth(); ++x) {
            if(level.GetCell(x, y) == type) {
                horizontal_count++;
            } else {
                horizontal_count = 0;
            }

            if(horizontal_count == size.x) {
                good_points.SetCell(x - size.x + 1, y, 1);
                horizontal_count--;
            }
        }
    }

    // count verticals
    for(int x = 0; x < level.GetWidth(); ++x) {
        int vertical_count = 0;

        for(int y = 0; y < level.GetHeight(); ++y) {
            if(good_points.GetCell(x, y) == 1) {
                vertical_count++;
            } else {
                vertical_count = 0;
            }

            if(vertical_count == size.y) {
                positions.push_back(Position(x, y - size.y + 1));
                vertical_count--;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

inline
bool FloodFill(CMap &level, Position position, int value, bool diagonal = true, int gradient = 0, RL::Position end = RL::Position(-1, -1))
{
    // flood fill room
    int area_value = level.GetCell(position.x, position.y);
    level.SetCell(position.x, position.y, value);

    std::list < Position > positions;
    positions.push_back(position);

    std::list < Position > ::iterator m, _m;
    m = positions.begin();

    while(m != positions.end()) {

        // Fill only to the end?
        if(end.x != -1 && end == (*m)) {
            break;
        }

        int pos_x = (*m).x;
        int pos_y = (*m).y;

        int this_value = level.GetCell(pos_x, pos_y);

        if(pos_x > 0)
            if(level.GetCell(pos_x - 1, pos_y) == area_value) {
                level.SetCell(pos_x - 1, pos_y, this_value + gradient);
                positions.push_back(Position(pos_x - 1, pos_y));
            }

        if(pos_x < (int) level.GetWidth() - 1)
            if(level.GetCell(pos_x + 1, pos_y) == area_value) {
                level.SetCell(pos_x + 1, pos_y, this_value + gradient);
                positions.push_back(Position(pos_x + 1, pos_y));
            }

        if(pos_y > 0)
            if(level.GetCell(pos_x, pos_y - 1) == area_value) {
                level.SetCell(pos_x, pos_y - 1, this_value + gradient);
                positions.push_back(Position(pos_x, pos_y - 1));
            }

        if(pos_y < (int) level.GetHeight() - 1)
            if(level.GetCell(pos_x, pos_y + 1) == area_value) {
                level.SetCell(pos_x, pos_y + 1, this_value + gradient);
                positions.push_back(Position(pos_x, pos_y + 1));
            }

        if(diagonal) {
            if(pos_x > 0 && pos_y > 0)
                if(level.GetCell(pos_x - 1, pos_y - 1) == area_value) {
                    level.SetCell(pos_x - 1, pos_y - 1, this_value + gradient);
                    positions.push_back(Position(pos_x - 1, pos_y - 1));
                }

            if(pos_x < (int) level.GetWidth() - 1 && pos_y < (int) level.GetHeight() - 1)
                if(level.GetCell(pos_x + 1, pos_y + 1) == area_value) {
                    level.SetCell(pos_x + 1, pos_y + 1, this_value + gradient);
                    positions.push_back(Position(pos_x + 1, pos_y + 1));
                }

            if(pos_x < (int) level.GetWidth() - 1 && pos_y > 0)
                if(level.GetCell(pos_x + 1, pos_y - 1) == area_value) {
                    level.SetCell(pos_x + 1, pos_y - 1, this_value + gradient);
                    positions.push_back(Position(pos_x + 1, pos_y - 1));
                }

            if(pos_x > 0 && pos_y < (int) level.GetHeight() - 1)
                if(level.GetCell(pos_x - 1, pos_y + 1) == area_value) {
                    level.SetCell(pos_x - 1, pos_y + 1, this_value + gradient);
                    positions.push_back(Position(pos_x - 1, pos_y + 1));
                }
        }

        m = positions.erase(m);
    }

    // filling to the end cell without success
    if(m == positions.end()) {
        return false;
    }

    if(end.x != -1 && end != (*m)) {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////

inline
bool FindOnMapRandomRectangleOfType(CMap &level, const RL::ELevelElement& type, RL::Position& pos, const RL::Size &size)
{
    std::vector < Position > positions;
    FindOnMapAllRectanglesOfType(level, type, size, positions);

    if(positions.size() == 0) {
        return false;
    }

    // get position of Random rectangle

    pos = positions[Random((int) positions.size())];
    return true;
}

//////////////////////////////////////////////////////////////////////////

inline
int CountNeighboursOfType(CMap &level, ELevelElement type, const Position& pos, bool diagonal = true)
{
    int neighbours = 0;

    if(pos.y > 0)
        if(level.GetCell(pos.x, pos.y - 1) == type) { // N
            neighbours++;
        }

    if(pos.x < (int) level.GetWidth() - 1)
        if(level.GetCell(pos.x + 1, pos.y) == type) { // E
            neighbours++;
        }

    if(pos.x > 0 && pos.y < (int) level.GetHeight() - 1)
        if(level.GetCell(pos.x, pos.y + 1) == type) { // S
            neighbours++;
        }

    if(pos.x > 0 && pos.y > 0)
        if(level.GetCell(pos.x - 1, pos.y) == type) { // W
            neighbours++;
        }

    if(diagonal) {
        if(pos.x > 0 && pos.y > 0)
            if(level.GetCell(pos.x - 1, pos.y - 1) == type) { // NW
                neighbours++;
            }

        if(pos.x < (int) level.GetWidth() - 1 && pos.y > 0)
            if(level.GetCell(pos.x + 1, pos.y - 1) == type) { // NE
                neighbours++;
            }

        if(pos.x < (int) level.GetWidth() - 1 && pos.y < (int) level.GetHeight() - 1) // SE
            if(level.GetCell(pos.x + 1, pos.y + 1) == type) {
                neighbours++;
            }


        if(pos.x > 0 && pos.y < (int) level.GetHeight() - 1)
            if(level.GetCell(pos.x - 1, pos.y + 1) == type) { // SW
                neighbours++;
            }
    }

    return neighbours;
}

//////////////////////////////////////////////////////////////////////////

inline
void AddDoors(CMap &level, float door_probability, float open_probability)
{
    for(size_t x = 0; x < level.GetWidth(); ++x)
        for(size_t y = 0; y < level.GetHeight(); ++y) {
            Position pos(x, y);
            int room_cells = CountNeighboursOfType(level, LevelElementRoom, pos);
            int corridor_cells = CountNeighboursOfType(level, LevelElementCorridor, pos);
            int open_door_cells = CountNeighboursOfType(level, LevelElementDoorOpen, pos);
            int close_door_cells = CountNeighboursOfType(level, LevelElementDoorClose, pos);
            int door_cells = open_door_cells + close_door_cells;

            if(level.GetCell(x, y) == LevelElementCorridor) {
                if((corridor_cells == 1 && door_cells == 0 && room_cells > 0 && room_cells < 4) ||
                        (corridor_cells == 0 && door_cells == 0)) {
                    float exist = ((float) Random(1000)) / 1000;

                    if(exist < door_probability) {
                        float is_open = ((float) Random(1000)) / 1000;

                        if(is_open < open_probability) {
                            level.SetCell(x, y, LevelElementDoorOpen);
                        } else {
                            level.SetCell(x, y, LevelElementDoorClose);
                        }
                    }
                }
            }
        }
}

//////////////////////////////////////////////////////////////////////////

inline
bool AddCorridor(CMap &level, const int& start_x1, const int& start_y1, const int& start_x2, const int& start_y2, bool straight = false)
{
    if(!level.OnMap(start_x1, start_y1) || !level.OnMap(start_x2, start_y2)) {
        return false;
    }

    // we start from both sides
    int x1, y1, x2, y2;

    x1 = start_x1;
    y1 = start_y1;
    x2 = start_x2;
    y2 = start_y2;

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

    while(1) {
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

        if(level.GetCell(x1, y1) == LevelElementWall_value) {
            level.SetCell(x1, y1, LevelElementCorridor_value);
        }

        if(level.GetCell(x2, y2) == LevelElementWall_value) {
            level.SetCell(x2, y2, LevelElementCorridor_value);
        }

        // connect corridors if on the same level
        if(x1 == x2) {
            while(y1 != y2) {
                y1 += dir_y;

                if(level.GetCell(x1, y1) == LevelElementWall_value) {
                    level.SetCell(x1, y1, LevelElementCorridor_value);
                }
            }

            if(level.GetCell(x1, y1) == LevelElementWall_value) {
                level.SetCell(x1, y1, LevelElementCorridor_value);
            }

            return true;
        }

        if(y1 == y2) {
            while(x1 != x2) {
                x1 += dir_x;

                if(level.GetCell(x1, y1) == LevelElementWall_value) {
                    level.SetCell(x1, y1, LevelElementCorridor_value);
                }
            }

            if(level.GetCell(x1, y1) == LevelElementWall_value) {
                level.SetCell(x1, y1, LevelElementCorridor_value);
            }

            return true;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////

inline
int FillDisconnectedRoomsWithDifferentValues(CMap &level)
{
    for(unsigned int y = 0; y < level.GetHeight(); ++y)
        for(unsigned int x = 0; x < level.GetWidth(); ++x) {
            if(level.GetCell(x, y) == LevelElementRoom) {
                level.SetCell(x, y, LevelElementRoom_value);
            } else if(level.GetCell(x, y) == LevelElementWall) {
                level.SetCell(x, y, LevelElementWall_value);
            }
        }

    int room_number = 0;

    for(size_t y = 0; y < level.GetHeight(); ++y) {
        for(size_t x = 0; x < level.GetWidth(); ++x) {
            if(level.GetCell(x, y) == LevelElementRoom_value) {
                FloodFill(level, Position(x, y), room_number++);
            }
        }
    }

    return room_number;
}

//////////////////////////////////////////////////////////////////////////

inline
void ConnectClosestRooms(CMap &level, bool with_doors, bool straight_connections = false)
{
    FillDisconnectedRoomsWithDifferentValues(level);

    std::vector < std::list < Position > > rooms;

    for(size_t y = 0; y < level.GetHeight(); ++y) {
        for(size_t x = 0; x < level.GetWidth(); ++x) {
            if(level.GetCell(x, y) != LevelElementWall_value) {
                if(level.GetCell(x, y) >= (int) rooms.size()) {
                    rooms.resize(level.GetCell(x, y) +1);
                }

                // only border cells without diagonals
                if(CountNeighboursOfType(level, LevelElementWall_value, Position(x, y), false) > 0) {
                    rooms[level.GetCell(x, y)].push_back(Position(x, y));
                }
            }
        }
    }

    Shuffle(rooms.begin(), rooms.end());

    if(rooms.size() < 2) {
        return;
    }

    // for warshall algorithm
    // set the connection matrix

    std::vector < std::vector < bool > > room_connections;
    std::vector < std::vector < bool > > transitive_closure;
    std::vector < std::vector < int > > distance_matrix;
    std::vector < std::vector < std::pair < Position, Position > > > closest_cells_matrix;

    room_connections.resize(rooms.size());
    transitive_closure.resize(rooms.size());
    distance_matrix.resize(rooms.size());
    closest_cells_matrix.resize(rooms.size());

    for(size_t a = 0; a < rooms.size(); ++a) {
        room_connections[a].resize(rooms.size());
        transitive_closure[a].resize(rooms.size());
        distance_matrix[a].resize(rooms.size());
        closest_cells_matrix[a].resize(rooms.size());

        for(size_t b = 0; b < rooms.size(); ++b) {
            room_connections[a][b] = false;
            distance_matrix[a][b] = INT_MAX;
        }
    }

    // find the closest cells for each room - Random closest cell
    std::list < Position >::iterator m, _m, k, _k;

    for(int room_a = 0; room_a < (int) rooms.size(); ++room_a) {
        for(int room_b = 0; room_b < (int) rooms.size(); ++room_b) {
            if(room_a == room_b) {
                continue;
            }

            std::pair < Position, Position > closest_cells;

            for(m = rooms[room_a].begin(), _m = rooms[room_a].end(); m != _m; ++m) {
                // for each boder cell in room_a try each border cell of room_b
                int x1 = (*m).x;
                int y1 = (*m).y;

                for(k = rooms[room_b].begin(), _k = rooms[room_b].end(); k != _k; ++k) {
                    int x2 = (*k).x;
                    int y2 = (*k).y;

                    int dist_ab = Distance(x1, y1, x2, y2);

                    if(dist_ab < distance_matrix[room_a][room_b] || (dist_ab == distance_matrix[room_a][room_b] && CoinToss())) {
                        closest_cells = std::make_pair(Position(x1, y1), Position(x2, y2));
                        distance_matrix[room_a][room_b] = dist_ab;
                    }
                }
            }

            closest_cells_matrix[room_a][room_b] = closest_cells;
        }
    }

    // Now connect the rooms to the closest ones
    for(int room_a = 0; room_a < (int) rooms.size(); ++room_a) {
        int min_distance = INT_MAX;
        int closest_room;

        for(int room_b = 0; room_b < (int) rooms.size(); ++room_b) {
            if(room_a == room_b) {
                continue;
            }

            int distance = distance_matrix[room_a][room_b];

            if(distance < min_distance) {
                min_distance = distance;
                closest_room = room_b;
            }
        }

        // connect room_a to closest one
        std::pair < Position, Position > closest_cells;
        closest_cells = closest_cells_matrix[room_a][closest_room];

        int x1 = closest_cells.first.x;
        int y1 = closest_cells.first.y;
        int x2 = closest_cells.second.x;
        int y2 = closest_cells.second.y;

        if(room_connections[room_a][closest_room] == false && AddCorridor(level, x1, y1, x2, y2, straight_connections)) {
            room_connections[room_a][closest_room] = true;
            room_connections[closest_room][room_a] = true;
        }
    }

    // The closest rooms connected. Connect the rest until all areas are connected

    for(int to_connect_a = 0; to_connect_a != -1;) {
        size_t a, b, c;
        int to_connect_b;

        for(a = 0; a < rooms.size(); a++)
            for(b = 0; b < rooms.size(); b++) {
                transitive_closure[a][b] = room_connections[a][b];
            }

        for(a = 0; a < rooms.size(); a++) {
            for(b = 0; b < rooms.size(); b++) {
                if(transitive_closure[a][b] == true && a != b) {
                    for(c = 0; c < rooms.size(); c++) {
                        if(transitive_closure[b][c] == true) {
                            transitive_closure[a][c] = true;
                            transitive_closure[c][a] = true;
                        }
                    }
                }
            }
        }

        // Check if all rooms are connected
        to_connect_a = -1;

        for(a = 0; a < rooms.size() && to_connect_a == -1; ++a) {
            for(b = 0; b < rooms.size(); b++) {
                if(a != b && transitive_closure[a][b] == false) {
                    to_connect_a = (int) a;
                    break;
                }
            }
        }

        if(to_connect_a != -1) {
            // connect rooms a & b
            do {
                to_connect_b = Random((int) rooms.size());
            } while(to_connect_b == to_connect_a);

            std::pair < Position, Position > closest_cells;
            closest_cells = closest_cells_matrix[to_connect_a][to_connect_b];

            int x1 = closest_cells.first.x;
            int y1 = closest_cells.first.y;
            int x2 = closest_cells.second.x;
            int y2 = closest_cells.second.y;

            AddCorridor(level, x1, y1, x2, y2, straight_connections);

            room_connections[to_connect_a][to_connect_b] = true;
            room_connections[to_connect_b][to_connect_a] = true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

inline
void AddRecursiveRooms(CMap &level, const RL::ELevelElement& type, int min_size_x, int min_size_y, RL::SRoom room, bool with_doors = true)
{
    int size_x = room.corner2.x - room.corner1.x;

    if(size_x % 2 != 0) {
        size_x -= CoinToss();
    }

    int size_y = room.corner2.y - room.corner1.y;

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

        int split = size_y / 2 + Random(size_y / 2 - min_size_y);

        for(int x = room.corner1.x; x < room.corner2.x; x++) {
            level.SetCell(x, room.corner1.y + split, type);
        }

        if(with_doors) {
            level.SetCell(room.corner1.x + Random(size_x - 1) +1, room.corner1.y + split, LevelElementDoorClose);
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

        int split = size_x / 2 + Random(size_x / 2 - min_size_x);

        for(int y = room.corner1.y; y < room.corner2.y; y++) {
            level.SetCell(room.corner1.x + split, y, type);
        }

        if(with_doors) {
            level.SetCell(room.corner1.x + split, room.corner1.y + Random(size_y - 1) +1, LevelElementDoorClose);
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

inline
void ConvertValuesToTiles(CMap &level)
{
    for(unsigned int y = 0; y < level.GetHeight(); ++y) {
        for(unsigned int x = 0; x < level.GetWidth(); ++x) {
            if(level.GetCell(x, y) == LevelElementCorridor_value) {
                level.SetCell(x, y, LevelElementCorridor);
            } else if(level.GetCell(x, y) == LevelElementWall_value) {
                level.SetCell(x, y, LevelElementWall);
            } else {
                level.SetCell(x, y, LevelElementRoom);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

inline
void DrawRectangleOnMap(CMap &level, const Position& p1, const Position& p2, int value)
{
    for(int y = p1.y; y < p2.y; ++y)
        for(int x = p1.x; x < p2.x; ++x) {
            level.SetCell(x, y, value);
        }
}

}

#endif

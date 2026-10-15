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
import rl.tile;
import std;

namespace RL
{

// Internal Math and Path Helpers
namespace detail
{
int Sqr(int x)
{
    return x * x;
}

void CutCorners(std::vector<Position>& seq)
{
    if (seq.size() < 3) {
        return;
    }

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

int SignCos2(const Position& p0, const Position& p1, const Position& p2)
{
    int sqlen01 = Sqr(static_cast<int>(p1.x - p0.x)) + Sqr(static_cast<int>(p1.y - p0.y));
    int sqlen12 = Sqr(static_cast<int>(p2.x - p1.x)) + Sqr(static_cast<int>(p2.y - p1.y));

    if (sqlen01 == 0 || sqlen12 == 0) {
        return 0;
    }

    int prod = static_cast<int>((p1.x - p0.x) * (p2.x - p1.x) + (p1.y - p0.y) * (p2.y - p1.y));
    long long prod_ll = prod; // Prevent overflow
    long long val = 1000LL * (prod_ll * prod_ll / sqlen01) / sqlen12;

    if (prod < 0) {
        val = -val;
    }

    return static_cast<int>(val);
}

void PerturbPath(std::vector<Position>& way, const CMap& level, int mindist, int maxdist, int pertamt)
{
    if (way.size() < 3) {
        return;
    }

    static const int Xoff[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int Yoff[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    const int mincos2 = 500;

    int mind2 = Sqr(mindist);
    int maxd2 = Sqr(maxdist);

    std::size_t loops = static_cast<std::size_t>(pertamt) * way.size();

    for (std::size_t i = 0; i < loops; ++i) {
        std::size_t ri = 1 + Random(static_cast<int>(way.size()) - 2);
        int rdir = Random(8);
        int nx = static_cast<int>(way[ri].x) + Xoff[rdir];
        int ny = static_cast<int>(way[ri].y) + Yoff[rdir];

        if (nx < 1 || nx >= static_cast<int>(level.getWidth()) - 1 ||
                ny < 1 || ny >= static_cast<int>(level.getHeight()) - 1) {
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

void ConnectWaypoints(std::vector<Position>& result, const std::vector<Position>& waypts)
{
    result.clear();

    if (waypts.size() <= 1) {
        return;
    }

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
export namespace RL
{

void FindOnMapAllRectanglesOfType(CMap &level, const std::string_view type, const Size &size, std::vector <Position>& positions)
{
    auto good_points = CMatrix<bool>(level.getSize(), false);

    for (std::size_t y = 0; y < level.getHeight(); ++y) {
        std::size_t horizontal_count = 0;

        for (std::size_t x = 0; x < level.getWidth(); ++x) {
            if (level.get(x, y).getType() == type) {
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
    for (std::size_t x = 0; x < level.getWidth(); ++x) {
        std::size_t vertical_count = 0;

        for (std::size_t y = 0; y < level.getHeight(); ++y) {
            if (good_points.get(x, y)) {
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

    if (positions.empty()) {
        return false;
    }

    // get position of Random rectangle
    pos = positions[Random(static_cast<std::size_t>(positions.size()))];
    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void AddDoors(CMap &level, float door_probability, float open_probability)
{
    for (std::size_t x = 0; x < level.getWidth(); ++x) {
        for (std::size_t y = 0; y < level.getHeight(); ++y) {
            Position pos(x, y);
            int room_cells = level.CountNeighbors(pos, CTile::ByType("room"));
            int corridor_cells = level.CountNeighbors(pos, CTile::ByType("corridor"));
            int open_door_cells = level.CountNeighbors(pos, CTile::ByType("door_open"));
            int close_door_cells = level.CountNeighbors(pos, CTile::ByType("door_closed"));
            int door_cells = open_door_cells + close_door_cells;

            if (level.get(x, y).getType() == "corridor") {
                if ((corridor_cells == 1 && door_cells == 0 && room_cells > 0 && room_cells < 4) ||
                        (corridor_cells == 0 && door_cells == 0)) {
                    float exist = (static_cast<float>(Random(1000))) / 1000.0f;

                    if (exist < door_probability) {
                        float is_open = (static_cast<float>(Random(1000))) / 1000.0f;

                        if (is_open < open_probability) {
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

bool AddCorridor(CMap &level, const std::size_t& start_x1, const std::size_t& start_y1, const std::size_t& start_x2, const std::size_t& start_y2, bool straight = false)
{
    if (!level.inside(start_x1, start_y1) || !level.inside(start_x2, start_y2)) {
        return false;
    }

    // we start from both sides
    std::size_t x1 = start_x1;
    std::size_t y1 = start_y1;
    std::size_t x2 = start_x2;
    std::size_t y2 = start_y2;

    int dir_x;
    int dir_y;

    if (start_x2 > start_x1) {
        dir_x = 1;
    } else {
        dir_x = -1;
    }

    if (start_y2 > start_y1) {
        dir_y = 1;
    } else {
        dir_y = -1;
    }

    // move into direction of the other end
    bool first_horizontal = CoinToss();
    bool second_horizontal = CoinToss();

    while (true) {
        if (!straight) {
            first_horizontal = CoinToss();
            second_horizontal = CoinToss();
        }

        if (x1 != x2 && y1 != y2) {
            if (first_horizontal) {
                x1 += dir_x;
            } else {
                y1 += dir_y;
            }
        }

        // connect rooms
        if (x1 != x2 && y1 != y2) {
            if (second_horizontal) {
                x2 -= dir_x;
            } else {
                y2 -= dir_y;
            }
        }

        if (!level.get(x1, y1).isPassable()) {
            level.SetCell(x1, y1, "corridor");
        }

        if (!level.get(x2, y2).isPassable()) {
            level.SetCell(x2, y2, "corridor");
        }

        // connect corridors if on the same level
        if (x1 == x2) {
            while (y1 != y2) {
                y1 += dir_y;

                if (!level.get(x1, y1).isPassable()) {
                    level.SetCell(x1, y1, "corridor");
                }
            }

            if (!level.get(x1, y1) .isPassable()) {
                level.SetCell(x1, y1, "corridor");
            }

            return true;
        }

        if (y1 == y2) {
            while (x1 != x2) {
                x1 += dir_x;

                if (!level.get(x1, y1).isPassable()) {
                    level.SetCell(x1, y1, "corridor");
                }
            }

            if (!level.get(x1, y1).isPassable()) {
                level.SetCell(x1, y1, "corridor");
            }

            return true;
        }
    } // while (true)
}

//////////////////////////////////////////////////////////////////////////

CMatrix<int> FillDisconnectedRoomsWithDifferentValues(const CMap &level)
{
    constexpr int any_room = -1;

    auto flood_map = CMatrix<int>(level.getSize(), 0);

    for (std::size_t y = 0; y < level.getHeight(); ++y) {
        for (std::size_t x = 0; x < level.getWidth(); ++x) {
            if (level.get(x, y).getType() == "room") {
                flood_map.set(x, y, any_room);
            }
        }
    }

    int room_number = 1;

    for (std::size_t y = 0; y < level.getHeight(); ++y) {
        for (std::size_t x = 0; x < level.getWidth(); ++x) {
            if (flood_map.get(x, y) == any_room) {
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
            if (floodmap.get(x, y) > 0) {
                // grow room list when required
                if (floodmap.get(x, y) >= static_cast<int>(rooms.size())) {
                    rooms.resize(floodmap.get(x, y) + 1);
                }

                // only border cells without diagonals
                if (floodmap.CountNeighbors(Position(x, y), 0) > 0) {
                    rooms[floodmap.get(x, y)].emplace_back(x, y);
                }
            } // if no wall at position
        } // for x
    } // for y

    if (rooms.size() < 2) {
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

            if (distance < min_distance) {
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

    if (size_x % 2 != 0) {
        size_x -= CoinToss();
    }

    std::size_t size_y = room.corner2.y - room.corner1.y;

    if (size_y % 2 != 0) {
        size_y -= CoinToss();
    }

    bool split_horizontal;

    if (size_y * 4 > size_x) {
        split_horizontal = true;
    } else if (size_x * 4 > size_y) {
        split_horizontal = false;
    } else {
        split_horizontal = CoinToss();
    }

    if (split_horizontal) { // split horizontal
        if (size_y / 2 < min_size_y) {
            return;
        }

        const std::size_t split = size_y / 2 + Random(size_y / 2 - min_size_y);

        for (std::size_t x = room.corner1.x; x < room.corner2.x; x++) {
            level.SetCell(x, room.corner1.y + split, type);
        }

        if (with_doors) {
            level.SetCell(room.corner1.x + Random(size_x - 1) + 1, room.corner1.y + split, "door_closed");
        }

        SRoom new_room = room;
        new_room.corner2.y = room.corner1.y + split;
        AddRecursiveRooms(level, type, min_size_x, min_size_y, new_room, with_doors);

        new_room = room;
        new_room.corner1.y = room.corner1.y + split;
        AddRecursiveRooms(level, type, min_size_x, min_size_y, new_room, with_doors);
    } else {
        if (size_x / 2 < min_size_x) {
            return;
        }

        const std::size_t split = size_x / 2 + Random(size_x / 2 - min_size_x);

        for (std::size_t y = room.corner1.y; y < room.corner2.y; y++) {
            level.SetCell(room.corner1.x + split, y, type);
        }

        if (with_doors) {
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
    for (std::size_t y = p1.y; y < p2.y; ++y) {
        for (std::size_t x = p1.x; x < p2.x; ++x) {
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
 */

/** Build a gently winding path using spaced, perturbed waypoints.
 *
 * Unlike a zigzag/sigsag path, it can wind even when its endpoints share
 * a row or column. Waypoint turns are limited to 45 degrees. The returned
 * tiles are eight-neighbour connected; raster stair steps still occur.
 * The map supplies bounds only: callers must check terrain collisions.
 * pertamt is a nonnegative perturbation amount (typically 5–50).
 */
bool BuildWindingPath(std::vector<Position>& road, const CMap& level,
                      const Position& start, const Position& end, int pertamt)
{
    road.clear();
    if (!level.inside(start) || !level.inside(end) || pertamt < 0) {
        return false;
    }
    const auto straight = start.BuildBresenhamLine(end);
    if (straight.size() < 5) {
        road = straight;
        return true;
    }
    std::vector<Position> waypoints{straight.front()};
    for (std::size_t i = 2 + Random(2); i + 2 < straight.size(); i += 2 + Random(2)) {
        waypoints.push_back(straight[i]);
    }
    // Append the endpoint once. The old sampler repeatedly selected the
    // last index without advancing, so paths of five tiles or more hung.
    waypoints.push_back(straight.back());
    detail::PerturbPath(waypoints, level, 2, 5, pertamt);
    detail::ConnectWaypoints(road, waypoints);
    detail::CutCorners(road);
    return true;
}

/** Route near a winding guide while respecting caller-supplied terrain costs.
 * A cost <= 0 blocks a tile. Positive costs prefer cheaper terrain.
 * Both endpoints are retained; no map tiles are changed. Failure clears road.
 */
bool BuildWindingRoute(std::vector<Position>& road, const CMap& level,
                       const Position& start, const Position& end, int pertamt,
                       const std::function<int(Position)>& terrain_cost)
{
    road.clear();
    std::vector<Position> guide;
    if (!BuildWindingPath(guide, level, start, end, pertamt) ||
            terrain_cost(start) <= 0 || terrain_cost(end) <= 0) return false;
    const auto width = level.getWidth(), count = width * level.getHeight();
    const auto index = [width](Position p) { return p.y * width + p.x; };
    std::vector<int> guide_distance(count, 8);
    std::vector<Position> queue;
    for (const auto p : guide) {
        if (guide_distance[index(p)] != 0) {
            guide_distance[index(p)] = 0;
            queue.push_back(p);
        }
    }
    for (std::size_t i = 0; i < queue.size(); ++i) {
        const auto here = queue[i];
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            const Position next(here.x + dx, here.y + dy);
            if (!level.inside(next)) continue;
            const auto distance = guide_distance[index(here)] + 1;
            if (distance < guide_distance[index(next)]) {
                guide_distance[index(next)] = distance;
                queue.push_back(next);
            }
        }
    }
    std::vector<std::int64_t> distance(count, std::numeric_limits<std::int64_t>::max());
    std::vector<std::size_t> parent(count, count);
    using Entry = std::pair<std::int64_t, std::size_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
    distance[index(start)] = 0;
    pending.emplace(0, index(start));
    while (!pending.empty()) {
        const auto [value, cell] = pending.top();
        pending.pop();
        if (value != distance[cell]) continue;
        if (cell == index(end)) break;
        const Position here(cell % width, cell / width);
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            const Position next(here.x + dx, here.y + dy);
            if (!level.inside(next)) continue;
            const int cost = terrain_cost(next);
            if (cost <= 0) continue;
            const auto next_cell = index(next);
            const auto candidate = value + std::int64_t(cost) * (dx && dy ? 14 : 10) +
                                   guide_distance[next_cell] * 6;
            if (candidate < distance[next_cell]) {
                distance[next_cell] = candidate;
                parent[next_cell] = cell;
                pending.emplace(candidate, next_cell);
            }
        }
    }
    if (distance[index(end)] == std::numeric_limits<std::int64_t>::max()) return false;
    for (auto cell = index(end);; cell = parent[cell]) {
        road.emplace_back(cell % width, cell / width);
        if (cell == index(start)) break;
    }
    std::ranges::reverse(road);
    return true;
}

/** Draw a winding road without large sharp turns.
 *
 * Uses BuildWindingPath; pertamt typically ranges from 5 to 50. This
 * drawing helper overwrites terrain and leaves the outer map edge alone.
 */
bool AddWindingRoad(CMap& level, const Position& start, const Position& end,
                    int pertamt, std::string_view tile_type = "corridor")
{
    std::vector<Position> road;
    if (!BuildWindingPath(road, level, start, end, pertamt)) {
        return false;
    }
    for (const auto& pos : road) {
        if (pos.x >= 1 && pos.x < level.getWidth() - 1 &&
                pos.y >= 1 && pos.y < level.getHeight() - 1) {
            level.SetCell(pos.x, pos.y, tile_type);
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////

/** Build a zigzag path between two points.
 *
 * This method is not aware of the map and usually only used as internal
 * helper for the highlevel-functions @ref AddZigzagRoad and @ref AddSigsagRoad
 * but may be useful for other uses.
 *
 * @param[out] ret All @ref Positions that form the road.
 * @param p1 The starting position.
 * @param p2 The final position.
 * @param turnpct The chance of a non-forced turn in percent (so, 100/turnpct
 *                is approximately the length of a straight segment.
 * @param diagpct The chance that a diagonal turn is allowed.
 */
void BuildZigzagPath(
    std::vector<Position>& ret,
    const Position& p1,
    const Position& p2,
    unsigned turnpct,
    unsigned diagpct
)
{
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

/** Build a zigzag route with individual grid corners cut diagonally.
 * This trims one-cell corners; it does not smooth the larger zigzag bends.
 * Use BuildWindingPath for gently winding routes with limited turn angles.
 */
void BuildSigsagPath(
    std::vector<Position>& ret,
    const Position& p1,
    const Position& p2,
    unsigned turnpct,
    unsigned diagpct
)
{
    BuildZigzagPath(ret, p1, p2, turnpct, diagpct);
    detail::CutCorners(ret);
}

/** Generates a randomly zigzagging road.
 *
 *  The road zigzags only if the endpoints differ in both coordinates,
 *  Otherwise it is a straight line.
 *
 * @param turnpct The chance of a non-forced turn in percent (so, 100/turnpct
 *                is approximately the length of a straight segment.
 * @param diagpct The chance that a diagonal turn is allowed.
 * @return Whether the path generation was successful.
 */
bool AddZigzagRoad(
    CMap &level,
    const Position& start,
    const Position& end,
    int turnpct,
    int diagpct,
    std::string_view tile_type = "corridor"
)
{
    if (!level.inside(start) || !level.inside(end)) {
        return false;
    }

    std::vector<Position> road;
    BuildZigzagPath(road, start, end, turnpct, diagpct);

    for (const auto& pos : road) {
        if (pos.x >= 1 && pos.x < level.getWidth() - 1 &&
                pos.y >= 1 && pos.y < level.getHeight() - 1) {
            level.SetCell(pos.x, pos.y, tile_type);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////

/** Draw a zigzag route with individual grid corners cut diagonally.
 * Large bends remain angular; use AddWindingRoad for gentle winding.
 */
bool AddSigsagRoad(
    CMap &level,
    const Position& start,
    const Position& end,
    int turnpct,
    int diagpct,
    std::string_view tile_type = "corridor"
)
{
    if (!level.inside(start) || !level.inside(end)) {
        return false;
    }

    std::vector<Position> road;
    BuildZigzagPath(road, start, end, turnpct, diagpct);
    detail::CutCorners(road);

    for (const auto& pos : road) {
        if (pos.x >= 1 && pos.x < level.getWidth() - 1 &&
                pos.y >= 1 && pos.y < level.getHeight() - 1) {
            level.SetCell(pos.x, pos.y, tile_type);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// End of Kusigrosz' road algorithms
//////////////////////////////////////////////////////////////////////////

} // end of export namespace RL

// Stamp a town (grass leaves the landscape intact), then join its local
// gate positions to surviving corridor/bridge tiles in the landscape.
// Returns the number of gates connected. No road network means no links.
export namespace RL
{
std::size_t StampTownWithRoads(CMap& landscape, const CMap& town, Position origin,
                              std::span<const Position> gates)
{
    if (!landscape.inside(origin) || town.getWidth() == 0 || town.getHeight() == 0) {
        return 0;
    }
    if (&landscape == &town) {
        const CMap snapshot(town);
        return StampTownWithRoads(landscape, snapshot, origin, gates);
    }
    const auto width = landscape.getWidth(), height = landscape.getHeight();
    const auto stamp_width = std::min(town.getWidth(), width - origin.x);
    const auto stamp_height = std::min(town.getHeight(), height - origin.y);
    std::vector<bool> protected_cell(width * height);
    for (std::size_t y = 0; y < stamp_height; ++y) {
        for (std::size_t x = 0; x < stamp_width; ++x) {
            protected_cell[(origin.y + y) * width + origin.x + x] = town.get(x, y).getType() != "grass";
        }
    }
    const int unreachable = std::numeric_limits<int>::max();
    std::vector<int> distance(width * height, unreachable);
    std::vector<std::size_t> parent(width * height, width * height);
    using Entry = std::pair<int, std::size_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const auto cell = y * width + x;
            const auto type = landscape.get(x, y).getType();
            if (!protected_cell[cell] && (type == "corridor" || type == "bridge")) {
                distance[cell] = 0;
                pending.emplace(0, cell);
            }
        }
    }
    const std::array<std::string_view, 1> skip{"grass"};
    landscape.Stamp(town, origin, skip);
    auto neighbors = [&](std::size_t cell) {
        return std::array{cell % width > 0 ? cell - 1 : width * height,
                          cell % width + 1 < width ? cell + 1 : width * height,
                          cell >= width ? cell - width : width * height,
                          cell + width < width * height ? cell + width : width * height};
    };
    // Prefer clear ground, clearing trees when necessary and bridging water
    // only when useful. Buildings and the entire stamped town are protected.
    while (!pending.empty()) {
        const auto [cost, cell] = pending.top();
        pending.pop();
        if (cost != distance[cell]) {
            continue;
        }
        for (auto next : neighbors(cell)) {
            if (next >= width * height || protected_cell[next]) {
                continue;
            }
            const auto type = landscape.get(next % width, next / width).getType();
            const int step = type == "grass" || type == "corridor" || type == "bridge" ? 1 :
                             type == "plant" ? 2 : type == "tree" ? 4 : type == "rock" ? 8 :
                             type == "water" ? 12 : 0;
            if (step == 0 || cost > unreachable - step || cost + step >= distance[next]) {
                continue;
            }
            distance[next] = cost + step;
            parent[next] = cell;
            pending.emplace(distance[next], next);
        }
    }
    std::size_t connected = 0;
    for (auto gate : gates) {
        if (gate.x >= stamp_width || gate.y >= stamp_height || town.get(gate).getType() != "corridor") {
            continue;
        }
        const auto cell = (origin.y + gate.y) * width + origin.x + gate.x;
        auto best = width * height;
        for (auto next : neighbors(cell)) {
            if (next < width * height && !protected_cell[next] && distance[next] != unreachable &&
                    (best == width * height || distance[next] < distance[best])) {
                best = next;
            }
        }
        if (best == width * height) {
            continue;
        }
        ++connected;
        while (distance[best] > 0) {
            const auto type = landscape.get(best % width, best / width).getType();
            if (type == "corridor" || type == "bridge") {
                break; // Join a link already laid by an earlier gate.
            }
            landscape.SetCell(best % width, best / width, type == "water" ? "bridge" : "corridor");
            best = parent[best];
        }
    }
    return connected;
}
}

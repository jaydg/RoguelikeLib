module;
export module rl.mapgenerators.growthtown;
import rl.map;
import rl.position;
import rl.randomness;
import std;

namespace RL
{
namespace
{
struct TownRoom {
    int x, y, w = 1, h = 1, area = 1, ratio = 1;
};

// Inspired by Lopes et al., GAMEON 2010: rectangular seed growth followed
// by frontier filling. Coarse cells leave one tile for walls and doors.
void TownInterior(CMap& map, const SBuilding& building)
{
    const int inside_width = building.corner2.x - building.corner1.x - 1;
    const int inside_height = building.corner2.y - building.corner1.y - 1;
    // Small cottages need their floor space more than internal partitions.
    if (inside_width * inside_height <= 9) {
        for (auto y = building.corner1.y + 1; y < building.corner2.y; ++y) {
            for (auto x = building.corner1.x + 1; x < building.corner2.x; ++x) {
                map.SetCell(Position(x, y), "room");
            }
        }
        map.SetCell(building.door, "door_closed");
        return;
    }
    const int w = (building.corner2.x - building.corner1.x) / 2;
    const int h = (building.corner2.y - building.corner1.y) / 2;
    const int max_rooms = std::min(w * h, std::max(2, w * h / 2));
    const int count = std::min(max_rooms, 2 + static_cast<int>(Random(3)));
    std::vector<int> owner(w * h, -1);
    std::vector<TownRoom> rooms{{0, 0, 1, 1, 1, 3}};
    // The public room starts just inside the street-facing entrance.
    const int entrance_x = building.door.x == building.corner1.x ? 0 :
                           building.door.x == building.corner2.x ? w - 1 :
                           (building.door.x - building.corner1.x - 1) / 2;
    const int entrance_y = building.door.y == building.corner1.y ? 0 :
                           building.door.y == building.corner2.y ? h - 1 :
                           (building.door.y - building.corner1.y - 1) / 2;
    rooms[0].x = entrance_x;
    rooms[0].y = entrance_y;
    owner[entrance_y * w + entrance_x] = 0;
    for (int id = 1; id < count; ++id) {
        std::vector<int> free;
        for (int c = 0; c < w * h; ++c) {
            if (owner[c] < 0) {
                free.push_back(c);
            }
        }
        const int c = free[Random(free.size())];
        owner[c] = id;
        rooms.push_back({c % w, c / w, 1, 1, 1, 1 + static_cast<int>(Random(2))});
    }
    for (;;) {
        int selected = -1, direction = 0;
        std::vector<int> strip;
        for (int id = 0; id < count; ++id) {
            const auto& r = rooms[id];
            const int start = Random(4);
            for (int step = 0; step < 4; ++step) {
                const int d = (start + step) % 4;
                const bool vertical = d < 2;
                const int fixed = vertical ? r.x + (d == 0 ? -1 : r.w) : r.y + (d == 2 ? -1 : r.h);
                if (fixed < 0 || fixed >= (vertical ? w : h)) {
                    continue;
                }
                std::vector<int> cells;
                for (int n = 0; n < (vertical ? r.h : r.w); ++n) {
                    const int c = vertical ? (r.y + n) * w + fixed : fixed * w + r.x + n;
                    if (owner[c] >= 0) {
                        cells.clear();
                        break;
                    }
                    cells.push_back(c);
                }
                if (!cells.empty() && (selected < 0 || r.area * rooms[selected].ratio < rooms[selected].area * r.ratio)) {
                    selected = id;
                    direction = d;
                    strip = std::move(cells);
                }
            }
        }
        if (selected < 0) {
            break;
        }
        auto& r = rooms[selected];
        for (int c : strip) {
            owner[c] = selected;
            ++r.area;
        }
        if (direction < 2) {
            r.x -= direction == 0;
            ++r.w;
        } else {
            r.y -= direction == 2;
            ++r.h;
        }
    }
    // Fill gaps from adjacent rooms, keeping each room connected.
    for (;;) {
        int selected = -1, cell = -1;
        for (int c = 0; c < w * h; ++c) {
            if (owner[c] >= 0) {
                continue;
            }
            const std::array neighbors{c % w > 0 ? c - 1 : -1, c % w + 1 < w ? c + 1 : -1,
                                       c >= w ? c - w : -1, c + w < w * h ? c + w : -1};
            for (int n : neighbors) {
                if (n < 0 || owner[n] < 0) {
                    continue;
                }
                const int id = owner[n];
                if (selected < 0 || rooms[id].area * rooms[selected].ratio < rooms[selected].area * rooms[id].ratio) {
                    selected = id;
                    cell = c;
                }
            }
        }
        if (selected < 0) {
            break;
        }
        owner[cell] = selected;
        ++rooms[selected].area;
    }
    struct Boundary { int a, b; Position pos; };
    std::vector<Boundary> boundaries;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int id = owner[y * w + x];
            const Position p(building.corner1.x + 1 + 2 * x, building.corner1.y + 1 + 2 * y);
            map.SetCell(p, "room");
            for (int d = 0; d < 2; ++d) {
                if ((d == 0 && x + 1 == w) || (d == 1 && y + 1 == h)) {
                    continue;
                }
                const int other = owner[y * w + x + (d == 0 ? 1 : w)];
                const Position wall(p.x + (d == 0), p.y + (d == 1));
                if (id == other) {
                    map.SetCell(wall, "room");
                } else {
                    boundaries.push_back({id, other, wall});
                }
            }
            if (x + 1 < w && y + 1 < h && owner[y * w + x + 1] == id &&
                    owner[(y + 1) * w + x] == id && owner[(y + 1) * w + x + 1] == id) {
                map.SetCell(p.x + 1, p.y + 1, "room");
            }
        }
    }
    // Prefer public-room doors, then join components through shared walls.
    Shuffle(boundaries.begin(), boundaries.end());
    std::stable_sort(boundaries.begin(), boundaries.end(), [](const auto& a, const auto& b) {
        return (a.a == 0 || a.b == 0) > (b.a == 0 || b.b == 0);
    });
    std::vector<int> component(count);
    std::iota(component.begin(), component.end(), 0);
    for (const auto& edge : boundaries) {
        const int a = component[edge.a], b = component[edge.b];
        if (a != b) {
            map.SetCell(edge.pos, "door_closed");
            for (int& c : component) {
                if (c == b) {
                    c = a;
                }
            }
        }
    }
    map.SetCell(building.door, "door_closed");
}

// Join waypoints with cardinal steps; diagonals must not disconnect roads.
void TownRoad(CMap& map, Position from, Position to, int radius = 0)
{
    const auto line = from.BuildBresenhamLine(to);
    auto paint = [&](Position p) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                const auto x = p.x + dx, y = p.y + dy;
                if (x < map.getWidth() && y < map.getHeight()) {
                    map.SetCell(x, y, "corridor");
                }
            }
        }
    };
    paint(from);
    for (auto p : line) {
        if (p.x != from.x && p.y != from.y) {
            paint(Position(p.x, from.y));
        }
        paint(p);
        from = p;
    }
}
} // namespace

// Replaces the map with a walled market town. Returns at most the requested
// number of buildings, with inclusive wall corners and street-facing doors.
export std::vector<SBuilding> CreateGrowthTown(CMap& map, std::size_t number_of_buildings = 80,
                                                  std::vector<Position>* town_gates = nullptr)
{
    if (town_gates) {
        town_gates->clear();
    }
    map.Clear("grass");
    const auto width = map.getWidth(), height = map.getHeight();
    if (width == 0 || height == 0) {
        return {};
    }
    if (width < 19 || height < 19) {
        TownRoad(map, Position(0, height / 2), Position(width - 1, height / 2));
        TownRoad(map, Position(width / 2, 0), Position(width / 2, height - 1));
        return {};
    }
    const double cx = (width - 1) / 2.0, cy = (height - 1) / 2.0;
    const double rx = cx - 2, ry = cy - 2;
    const double phase = RandomFloat(0, 6.2831853f);
    std::vector<bool> inside(width * height);
    // A gently uneven oval avoids an axis-aligned town boundary.
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const double dx = (x - cx) / rx, dy = (y - cy) / ry;
            const double angle = std::atan2(dy, dx);
            const double edge = 0.94 + 0.045 * std::sin(3 * angle + phase) + 0.025 * std::cos(5 * angle - phase);
            inside[y * width + x] = dx * dx + dy * dy < edge * edge;
        }
    }
    // Overlap the stepped ramparts: use the whole eight-cell boundary,
    // so diagonal movement cannot squeeze between corner-touching walls.
    for (std::size_t y = 1; y + 1 < height; ++y) {
        for (std::size_t x = 1; x + 1 < width; ++x) {
            if (!inside[y * width + x]) {
                continue;
            }
            bool boundary = false;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    boundary |= !inside[(y + dy) * width + x + dx];
                }
            }
            if (boundary) {
                map.SetCell(x, y, "wall");
            }
        }
    }
    const Position market(width / 2 + Random(5) - 2, height / 2 + Random(5) - 2);
    for (auto y = market.y - 2; y <= market.y + 2; ++y) {
        for (auto x = market.x - 3; x <= market.x + 3; ++x) {
            map.SetCell(x, y, "corridor");
        }
    }
    // Four gate roads bend around offset waypoints on their way to market.
    const std::array gates{Position(0, height / 2), Position(width - 1, height / 2),
                           Position(width / 2, 0), Position(width / 2, height - 1)};
    for (const auto gate : gates) {
        const Position bend((market.x + gate.x) / 2 + Random(5) - 2,
                            (market.y + gate.y) / 2 + Random(5) - 2);
        TownRoad(map, market, bend, 1);
        TownRoad(map, bend, gate, 1);
    }
    // Main roads and the market are kept clear permanently. Lesser lanes
    // may be rerouted around a new house's one-cell perimeter passage.
    std::vector<bool> main_road(width * height);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            main_road[y * width + x] = map.get(x, y).getType() == "corridor";
        }
    }
    // An uneven inner loop connects the quarters without making grid blocks.
    constexpr int segments = 12;
    std::array<Position, segments> ring;
    for (int i = 0; i < segments; ++i) {
        const double angle = i * 6.283185307179586 / segments;
        const double scale = RandomFloat(0.55f, 0.69f);
        ring[i] = Position(static_cast<std::size_t>(cx + rx * scale * std::cos(angle)),
                           static_cast<std::size_t>(cy + ry * scale * std::sin(angle)));
    }
    for (int i = 0; i < segments; ++i) {
        TownRoad(map, ring[i], ring[(i + 1) % segments]);
    }
    // Tie the loop to the centre explicitly, even if bent gate roads miss it.
    TownRoad(map, market, ring[Random(segments)]);
    // Short winding lanes branch out into the quarters, creating varied plots.
    for (int i = 0; i < segments; i += 2) {
        const Position start = ring[i];
        const Position end((start.x + market.x) / 2 + Random(5) - 2,
                           (start.y + market.y) / 2 + Random(5) - 2);
        TownRoad(map, start, end);
    }
    // Keep the three-tile arteries inside the town, but narrow each
    // opening through the ramparts to two parallel rows or columns.
    std::array<Position, 4> gate_centres;
    auto boundary_cell = [&](std::size_t x, std::size_t y) {
        if (!inside[y * width + x]) {
            return false;
        }
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (!inside[(y + dy) * width + x + dx]) {
                    return true;
                }
            }
        }
        return false;
    };
    for (std::size_t i = 0; i < gates.size(); ++i) {
        auto nearest = std::numeric_limits<std::size_t>::max();
        for (std::size_t y = 1; y + 1 < height; ++y) {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                if (boundary_cell(x, y) && map.get(x, y).getType() == "corridor") {
                    const auto distance = Position(x, y).Distance(gates[i]);
                    if (distance < nearest) {
                        nearest = distance;
                        gate_centres[i] = Position(x, y);
                    }
                }
            }
        }
    }
    for (std::size_t y = 1; y + 1 < height; ++y) {
        for (std::size_t x = 1; x + 1 < width; ++x) {
            if (!boundary_cell(x, y)) {
                continue;
            }
            bool opening = false;
            for (std::size_t i = 0; i < gates.size(); ++i) {
                const auto p = gate_centres[i];
                if (i < 2) {
                    opening |= (i == 0 ? x < width / 2 : x >= width / 2) && (y == p.y || y == p.y + 1);
                } else {
                    opening |= (i == 2 ? y < height / 2 : y >= height / 2) && (x == p.x || x == p.x + 1);
                }
            }
            map.SetCell(x, y, opening ? "corridor" : "wall");
        }
    }
    // Carving a bent gate road can clip a rampart corner. Restore overlap
    // at those junctions too, without bridging the broad gate openings.
    bool repaired;
    do {
        repaired = false;
        for (std::size_t y = 0; y + 1 < height; ++y) {
            for (std::size_t x = 0; x + 1 < width; ++x) {
                const bool a = map.get(x, y).getType() == "wall";
                const bool b = map.get(x + 1, y).getType() == "wall";
                const bool c = map.get(x, y + 1).getType() == "wall";
                const bool d = map.get(x + 1, y + 1).getType() == "wall";
                if ((a && d && !b && !c) || (b && c && !a && !d)) {
                    const Position first = a ? Position(x + 1, y) : Position(x, y);
                    const Position second = a ? Position(x, y + 1) : Position(x + 1, y + 1);
                    auto distance = [&](Position p) {
                        const double dx = (p.x - cx) / rx, dy = (p.y - cy) / ry;
                        return dx * dx + dy * dy;
                    };
                    map.SetCell(distance(first) > distance(second) ? first : second, "wall");
                    repaired = true;
                }
            }
        }
    } while (repaired);
    // The market well is reserved before building placement. Quarter wells
    // are found afterwards, from the squares the buildings actually leave.
    map.SetCell(market, "well");

    struct Frontage { Position road; int dx, dy; };
    std::vector<Frontage> frontages;
    std::vector<bool> queued(width * height * 4);
    constexpr std::array<std::pair<int, int>, 4> directions{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    auto add_frontages = [&](std::size_t x, std::size_t y, bool back_plot = false) {
        const auto type = map.get(x, y).getType();
        if (!inside[y * width + x] || (type != "corridor" && !(back_plot && type == "grass"))) {
            return;
        }
        for (std::size_t d = 0; d < directions.size(); ++d) {
            const auto index = (y * width + x) * 4 + d;
            if (!queued[index]) {
                queued[index] = true;
                frontages.push_back({Position(x, y), directions[d].first, directions[d].second});
            }
        }
    };
    for (std::size_t y = 1; y + 1 < height; ++y) {
        for (std::size_t x = 1; x + 1 < width; ++x) {
            add_frontages(x, y);
        }
    }
    Shuffle(frontages.begin(), frontages.end());
    // Settle the market quarter first, then spread towards the town walls.
    std::stable_sort(frontages.begin(), frontages.end(), [&](const auto& a, const auto& b) {
        auto quarter = [&](Position p) {
            const double dx = (p.x - cx) / rx, dy = (p.y - cy) / ry;
            return static_cast<int>(std::sqrt(dx * dx + dy * dy) * 3);
        };
        return quarter(a.road) < quarter(b.road);
    });
    // Back plots follow the street-facing houses. Their clearance becomes
    // paved access at the end, so a plot need not touch an original road.
    const auto street_frontages = frontages.size();
    for (std::size_t y = 1; y + 1 < height; ++y) {
        for (std::size_t x = 1; x + 1 < width; ++x) {
            add_frontages(x, y, true);
        }
    }
    Shuffle(frontages.begin() + street_frontages, frontages.end());
    std::vector<SBuilding> buildings;
    // Fill street frontages first, then the remaining plots. Every house
    // keeps a one-cell perimeter passage shared with its neighbours.
    for (std::size_t index = 0; index < frontages.size() && buildings.size() < number_of_buildings; ++index) {
        const auto frontage = frontages[index];
        const auto access_type = map.get(frontage.road).getType();
        if (access_type != "corridor" && access_type != "grass") {
            continue;
        }
        const bool back_plot = index >= street_frontages;
        for (int attempt = 0; attempt < (back_plot ? 9 : 5); ++attempt) {
            // Compact cottages dominate. Small workshops and sheds fill
            // narrow back plots that cannot accommodate a five-tile house.
            int w = 4, h = 4;
            if (attempt < 3) {
                w = 2 * (2 + (Random(4) == 0 ? 1 : 0));
                h = 2 * (2 + (Random(4) == 0 ? 1 : 0));
            } else if (attempt >= 7) {
                w = h = 2;
            } else if (attempt == 5) {
                w = 2;
            } else if (attempt == 6) {
                h = 2;
            }
            const int frontage_length = frontage.dx == 0 ? w : h;
            const int offset = attempt < 3 ? 1 + 2 * Random(frontage_length / 2) :
                               1 + 2 * ((attempt - 3) % (frontage_length / 2));
            const int door_x = static_cast<int>(frontage.road.x) + frontage.dx;
            const int door_y = static_cast<int>(frontage.road.y) + frontage.dy;
            const int left = frontage.dx == 0 ? door_x - offset : door_x - (frontage.dx < 0 ? w : 0);
            const int top = frontage.dy == 0 ? door_y - offset : door_y - (frontage.dy < 0 ? h : 0);
            if (left < 1 || top < 1 || left + w + 1 >= static_cast<int>(width) || top + h + 1 >= static_cast<int>(height)) {
                continue;
            }
            bool fits = true;
            for (int y = top - 1; y <= top + h + 1 && fits; ++y) {
                for (int x = left - 1; x <= left + w + 1; ++x) {
                    const auto type = map.get(x, y).getType();
                    const bool footprint = x >= left && x <= left + w && y >= top && y <= top + h;
                    const bool occupied = footprint ? main_road[y * width + x] || (type != "grass" && type != "corridor") :
                                          type == "wall" || type == "door_closed" || type == "room";
                    if (!inside[y * width + x] || occupied) {
                        fits = false;
                        break;
                    }
                }
            }
            if (!fits) {
                continue;
            }
            SBuilding building;
            building.corner1 = Position(left, top);
            building.corner2 = Position(left + w, top + h);
            building.door = Position(door_x, door_y);
            for (int y = top; y <= top + h; ++y) {
                for (int x = left; x <= left + w; ++x) {
                    map.SetCell(x, y, "wall");
                }
            }
            TownInterior(map, building);
            // Adjacent houses share this clearance, leaving exactly one tile
            // between walls. The loop is linked to the entrance's street.
            for (int y = top - 1; y <= top + h + 1; ++y) {
                for (int x = left - 1; x <= left + w + 1; ++x) {
                    if (x == left - 1 || x == left + w + 1 || y == top - 1 || y == top + h + 1) {
                        if (map.get(x, y).getType() == "grass") {
                            map.SetCell(x, y, "corridor");
                        }
                    }
                }
            }
            buildings.push_back(building);
            break;
        }
    }
    // Paved courtyards and residual passages replace all urban grass.
    // Outside the ramparts the countryside remains green.
    std::vector<bool> vacant(width * height);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (inside[y * width + x] && map.get(x, y).getType() == "grass") {
                vacant[y * width + x] = true;
                map.SetCell(x, y, "corridor");
            }
        }
    }
    // A handful of tiny two-tile gardens sit in leftover pockets against
    // house walls. They never replace the main streets or an entrance path.
    std::vector<Position> gardens;
    for (std::size_t y = 1; y + 1 < height; ++y) {
        for (std::size_t x = 1; x + 2 < width; ++x) {
            if (!vacant[y * width + x] || !vacant[y * width + x + 1]) {
                continue;
            }
            const bool beside_house = std::ranges::any_of(buildings, [&](const auto& b) {
                return x > b.corner1.x && x + 1 < b.corner2.x &&
                       (y + 2 == b.corner1.y || y == b.corner2.y + 2);
            });
            if (beside_house) {
                gardens.emplace_back(x, y);
            }
        }
    }
    Shuffle(gardens.begin(), gardens.end());
    std::vector<Position> planted;
    for (const auto garden : gardens) {
        if (planted.size() >= buildings.size() / 10) {
            break;
        }
        if (std::ranges::any_of(planted, [&](Position p) {
                return std::abs(static_cast<int>(p.x) - static_cast<int>(garden.x)) <= 3 &&
                       std::abs(static_cast<int>(p.y) - static_cast<int>(garden.y)) <= 2;
            })) {
            continue;
        }
        map.SetCell(garden, "plant");
        map.SetCell(garden.x + 1, garden.y, "plant");
        planted.push_back(garden);
    }
    // Find the largest paved square in each quarter. A maximal-square
    // scan excludes the main roads, gardens and buildings, so narrow lanes
    // and the wide gate roads do not masquerade as public squares.
    struct Square { int side = 0; Position centre; double offset = 0; };
    std::array<Square, 4> squares;
    std::vector<int> side(width * height);
    auto quarter = [&](std::size_t x, std::size_t y) {
        return (y >= height / 2 ? 2 : 0) + (x >= width / 2 ? 1 : 0);
    };
    for (std::size_t y = 1; y < height; ++y) {
        for (std::size_t x = 1; x < width; ++x) {
            const auto cell = y * width + x;
            if (!inside[cell] || main_road[cell] || map.get(x, y).getType() != "corridor") {
                continue;
            }
            const int q = quarter(x, y);
            const int left = quarter(x - 1, y) == q ? side[cell - 1] : 0;
            const int above = quarter(x, y - 1) == q ? side[cell - width] : 0;
            const int diagonal = quarter(x - 1, y - 1) == q ? side[cell - width - 1] : 0;
            const int size = side[cell] = 1 + std::min({left, above, diagonal});
            if (size < 3) {
                continue;
            }
            const Position centre(x - size / 2, y - size / 2);
            const double dx = centre.x - (q % 2 == 0 ? width * 0.25 : width * 0.75);
            const double dy = centre.y - (q < 2 ? height * 0.25 : height * 0.75);
            const double offset = dx * dx + dy * dy;
            if (size > squares[q].side || (size == squares[q].side && offset < squares[q].offset)) {
                squares[q] = {size, centre, offset};
            }
        }
    }
    for (const auto& square : squares) {
        if (square.side >= 3) {
            map.SetCell(square.centre, "well");
        }
    }
    // Export the actual openings at the ramparts, rather than the temporary
    // edge waypoints used to draw the roads. Leave no road stubs outside.
    for (const auto edge : gates) {
        Position opening;
        std::size_t nearest = std::numeric_limits<std::size_t>::max();
        for (std::size_t y = 1; y + 1 < height; ++y) {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                if (!inside[y * width + x] || map.get(x, y).getType() != "corridor") {
                    continue;
                }
                if (inside[y * width + x - 1] && inside[y * width + x + 1] &&
                        inside[(y - 1) * width + x] && inside[(y + 1) * width + x]) {
                    continue;
                }
                const auto distance = Position(x, y).Distance(edge);
                if (distance < nearest) {
                    nearest = distance;
                    opening = Position(x, y);
                }
            }
        }
        if (town_gates && opening.x != Position::invalid) {
            town_gates->push_back(opening);
        }
    }
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (!inside[y * width + x] && map.get(x, y).getType() == "corridor") {
                map.SetCell(x, y, "grass");
            }
        }
    }
    return buildings;
}
} // namespace RL

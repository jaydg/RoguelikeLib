import rl.mapgenerators;
import rl.map;
import rl.position;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{
std::string Snapshot(const RL::CMap& map)
{
    std::string result;
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            result += std::format("{}:{};", map.get(x, y).getType(), map.get(x, y).getColor());
        }
    }
    return result;
}

void CheckTown(const RL::CMap& map, const std::vector<RL::SBuilding>& buildings)
{
    const auto w = map.getWidth(), h = map.getHeight();
    std::vector<bool> visited(w * h);
    std::vector<RL::Position> queue{RL::Position(0, 0)};
    visited[0] = true;
    for (std::size_t i = 0; i < queue.size(); ++i) {
        const auto p = queue[i];
        const std::array next{RL::Position(p.x - 1, p.y), RL::Position(p.x + 1, p.y),
                              RL::Position(p.x, p.y - 1), RL::Position(p.x, p.y + 1)};
        for (auto n : next) {
            if (n.x >= w || n.y >= h || visited[n.y * w + n.x]) {
                continue;
            }
            // Closed doors are traversable after opening them.
            if (map.get(n).isPassable() || map.get(n).getType() == "door_closed") {
                visited[n.y * w + n.x] = true;
                queue.push_back(n);
            }
        }
    }
    bool connected = true;
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            if (map.get(x, y).getType() == "room" || map.get(x, y).getType() == "corridor" ||
                    map.get(x, y).getType() == "door_closed") {
                connected &= visited[y * w + x];
            }
        }
    }
    Check(connected, "every room, door and street can be reached with cardinal steps");
    bool enclosed = true;
    for (const auto& b : buildings) {
        const auto outside = b.door.x == b.corner1.x ? RL::Position(b.door.x - 1, b.door.y) :
                             b.door.x == b.corner2.x ? RL::Position(b.door.x + 1, b.door.y) :
                             b.door.y == b.corner1.y ? RL::Position(b.door.x, b.door.y - 1) :
                             RL::Position(b.door.x, b.door.y + 1);
        Check(map.get(b.door).getType() == "door_closed" &&
              map.get(outside).getType() == "corridor", "entrance has a street approach");
        for (auto y = b.corner1.y; y <= b.corner2.y; ++y) {
            for (auto x = b.corner1.x; x <= b.corner2.x; ++x) {
                if (x == b.corner1.x || x == b.corner2.x || y == b.corner1.y || y == b.corner2.y) {
                    enclosed &= RL::Position(x, y) == b.door || map.get(x, y).getType() == "wall";
                }
            }
        }
    }
    Check(enclosed, "exterior walls are intact except at the entrance");
}

void CheckUrbanForm(const RL::CMap& map, const std::vector<RL::SBuilding>& buildings)
{
    const auto w = map.getWidth(), h = map.getHeight();
    std::vector<bool> rampart(w * h);
    bool paved = true;
    for (std::size_t y = 0; y < h; ++y) {
        std::size_t first = w, last = 0;
        for (std::size_t x = 0; x < w; ++x) {
            rampart[y * w + x] = map.get(x, y).getType() == "wall" &&
                std::ranges::none_of(buildings, [&](const auto& b) {
                    return x >= b.corner1.x && x <= b.corner2.x && y >= b.corner1.y && y <= b.corner2.y;
                });
            if (rampart[y * w + x]) {
                first = std::min(first, x);
                last = x;
            }
        }
        for (auto x = first; x < last; ++x) {
            paved &= map.get(x, y).getType() != "grass";
        }
    }
    Check(paved, "there are no grassy fields within the town walls");
    std::array<int, 4> quarter_wells{};
    int well_count = 0;
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            if (map.get(x, y).getType() == "well") {
                ++well_count;
                Check(map.get(x, y).getGlyph() == U'o' && !map.get(x, y).isPassable(), "wells use o and cannot be walked through");
                if (std::abs(static_cast<int>(x) - static_cast<int>(w / 2)) > 2 ||
                        std::abs(static_cast<int>(y) - static_cast<int>(h / 2)) > 2) {
                    ++quarter_wells[(y > h / 2 ? 2 : 0) + (x > w / 2 ? 1 : 0)];
                    bool paved_square = true;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx != 0 || dy != 0) {
                                paved_square &= map.get(x + dx, y + dy).getType() == "corridor";
                            }
                        }
                    }
                    Check(paved_square, "quarter wells occupy the centre of an existing paved square");
                }
            }
        }
    }
    Check(well_count >= 1 && well_count <= 5 && std::ranges::all_of(quarter_wells, [](int count) { return count <= 1; }),
          "one well serves the market, with at most one per quarter that has a square");
    auto remaining = rampart;
    int arcs = 0;
    for (std::size_t start = 0; start < w * h; ++start) {
        if (!remaining[start]) {
            continue;
        }
        ++arcs;
        std::vector<std::size_t> queue{start};
        remaining[start] = false;
        for (std::size_t i = 0; i < queue.size(); ++i) {
            const auto c = queue[i];
            const std::array next{c % w > 0 ? c - 1 : w * h, c % w + 1 < w ? c + 1 : w * h,
                                  c >= w ? c - w : w * h, c + w < w * h ? c + w : w * h};
            for (auto n : next) {
                if (n < w * h && remaining[n]) {
                    remaining[n] = false;
                    queue.push_back(n);
                }
            }
        }
    }
    Check(arcs == 4, "ramparts are four continuous wall arcs separated only by the gates");
    bool overlaps = true;
    for (std::size_t y = 0; y + 1 < h; ++y) {
        for (std::size_t x = 0; x + 1 < w; ++x) {
            const bool a = rampart[y * w + x], b = rampart[y * w + x + 1];
            const bool c = rampart[(y + 1) * w + x], d = rampart[(y + 1) * w + x + 1];
            overlaps &= !((a && d && !b && !c) || (b && c && !a && !d));
        }
    }
    Check(overlaps, "stepped ramparts overlap instead of leaving diagonal squeezing gaps");
    bool broad_gates = true;
    for (std::size_t offset = 0; offset < 3; ++offset) {
        broad_gates &= map.get(w / 2 - 1 + offset, 0).getType() == "corridor" &&
                       map.get(w / 2 - 1 + offset, h - 1).getType() == "corridor" &&
                       map.get(0, h / 2 - 1 + offset).getType() == "corridor" &&
                       map.get(w - 1, h / 2 - 1 + offset).getType() == "corridor";
    }
    Check(broad_gates, "all four main roads have three-tile-wide approaches");
    std::vector<bool> garden(w * h);
    for (std::size_t c = 0; c < w * h; ++c) {
        garden[c] = map.get(c % w, c / w).getType() == "plant";
    }
    Check(std::ranges::count(garden, true) <= static_cast<int>(2 * (buildings.size() / 10)), "only a handful of garden tiles are green");
    bool tiny = true;
    for (std::size_t c = 0; c < w * h; ++c) {
        if (garden[c]) {
            tiny &= c % w + 1 < w && garden[c + 1];
            garden[c] = false;
            if (c % w + 1 < w) {
                garden[c + 1] = false;
            }
        }
    }
    Check(tiny, "gardens consist of isolated two-tile plots");
    bool narrow_passage = false;
    for (const auto& a : buildings) {
        for (const auto& b : buildings) {
            if (a.corner2.x + 2 == b.corner1.x && std::max(a.corner1.y, b.corner1.y) <= std::min(a.corner2.y, b.corner2.y)) {
                narrow_passage = true;
            }
            if (a.corner2.y + 2 == b.corner1.y && std::max(a.corner1.x, b.corner1.x) <= std::min(a.corner2.x, b.corner2.x)) {
                narrow_passage = true;
            }
        }
    }
    Check(narrow_passage, "neighbouring houses share one-tile passages");
}
}

void TestGrowthTown()
{
    RL::CMap map(RL::Size(79, 50));
    std::array<bool, 4> orientations{};
    std::set<std::string> well_layouts;
    std::size_t house_count = 0, total_width = 0, total_height = 0;
    for (unsigned seed = 0; seed < 100; ++seed) {
        RL::InitRandomness(seed);
        const auto buildings = RL::CreateGrowthTown(map, 1000);
        Check(buildings.size() >= 30 && buildings.size() <= 1000, "fills a standard town with at least thirty buildings");
        CheckUrbanForm(map, buildings);
        std::string wells;
        for (std::size_t y = 0; y < map.getHeight(); ++y) {
            for (std::size_t x = 0; x < map.getWidth(); ++x) {
                if (map.get(x, y).getType() == "well" &&
                        (std::abs(static_cast<int>(x) - static_cast<int>(map.getWidth() / 2)) > 2 ||
                         std::abs(static_cast<int>(y) - static_cast<int>(map.getHeight() / 2)) > 2)) {
                    wells += std::format("{},{};", x, y);
                }
            }
        }
        well_layouts.insert(wells);
        CheckTown(map, buildings);
        std::size_t ramparts = 0;
        for (std::size_t y = 0; y < map.getHeight(); ++y) {
            for (std::size_t x = 0; x < map.getWidth(); ++x) {
                if (map.get(x, y).getType() == "wall" && std::ranges::none_of(buildings, [&](const auto& b) {
                        return x >= b.corner1.x && x <= b.corner2.x && y >= b.corner1.y && y <= b.corner2.y;
                    })) {
                    ++ramparts;
                }
            }
        }
        Check(ramparts > 80, "a substantial town wall survives street and house placement");
        for (const auto& b : buildings) {
            ++house_count;
            total_width += b.corner2.x - b.corner1.x + 1;
            total_height += b.corner2.y - b.corner1.y + 1;
            orientations[b.door.x == b.corner1.x ? 0 : b.door.x == b.corner2.x ? 1 : b.door.y == b.corner1.y ? 2 : 3] = true;
        }
    }
    Check(well_layouts.size() > 10, "quarter wells follow the generated squares instead of fixed coordinates");
    Check(total_width < house_count * 7 && total_height < house_count * 7, "the town is dominated by compact houses");
    Check(std::ranges::all_of(orientations, [](bool used) { return used; }), "houses face streets in all four directions");
    for (auto size : std::array{RL::Size(19, 19), RL::Size(19, 50), RL::Size(50, 19), RL::Size(120, 70)}) {
        for (unsigned seed = 0; seed < 20; ++seed) {
            RL::InitRandomness(seed);
            RL::CMap varied(size);
            const auto buildings = RL::CreateGrowthTown(varied, 1000);
            CheckTown(varied, buildings);
        }
    }
    RL::InitRandomness(42);
    RL::CreateGrowthTown(map, 4);
    const auto snapshot = Snapshot(map);
    RL::InitRandomness(42);
    Check(RL::CreateGrowthTown(map, 4).size() == 4 && Snapshot(map) == snapshot, "seed reproduces tiles and colours");
    Check(RL::CreateGrowthTown(map, 0).empty(), "zero buildings requested");
    CheckTown(map, {});
    for (std::size_t h = 0; h <= 12; ++h) {
        for (std::size_t w = 0; w <= 12; ++w) {
            RL::CMap small(RL::Size(w, h));
            const auto buildings = RL::CreateGrowthTown(small, 1000);
            Check(buildings.size() <= 1, "tiny maps terminate with at most one building");
            if (w > 0 && h > 0) {
                CheckTown(small, buildings);
            }
        }
    }
}

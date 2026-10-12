module;

//////////////////////////////////////////////////////////////////////////
// Voronoi Wilderness
//
// A wilderness grown from seed points scattered over the map: every cell
// belongs to the region of the point nearest to it, the Delaunay
// triangulation of the points is the adjacency graph of the regions, and a
// flood fill over that graph turns the distance from a starting region
// into a difficulty between 0 and 9. The difficulty decides the terrain of
// a region, and the terrain decides the tiles: open ground scattered with
// point terrain, the starting town at difficulty 0 with small buildings if asked for,
// farmland walled in by hedges and left a single gate, rolling hills with
// crags on their tops, mountains and high snow rounded and worn to a
// fractal outline, and lakes and chasms dropped in only where
// every passable region stays reachable.
//
// The shape of the algorithm is Andrew Doull's "Wilderness generation
// using Voronoi diagrams", written for Unangband and published at
// roguelikedeveloper.blogspot.com in July 2007.
//
//////////////////////////////////////////////////////////////////////////

export module rl.mapgenerators.voronoiwilderness;

export import rl.connectivity;
export import rl.locations;
export import rl.map;
import rl.matrix;
import rl.position;
import rl.randomness;
import rl.tile;
import std;

export namespace RL
{

// What the generator worked out, next to the tiles it set.
struct SWildernessInfo {
    // The Voronoi regions the map was divided into
    std::size_t regions = 0;

    // The edges of the Delaunay triangulation: which region neighbours which
    std::size_t adjacency = 0;

    // The places of significance the generator laid out, in the shape the
    // forest generator hands its clearings back in: the starting town, as
    // the deepest cell of the town region, with that cell's distance to
    // the region's edge as radius. Empty when no cell of the map belongs
    // to the town region.
    std::vector<SLocation> locations;

    // The buildings, when they were asked for: 4–7 tiles along each side,
    // including walls, with multiple houses where the town has room.
    // The rectangle of each
    // one's outer walls, corners included, with floor inside, and the
    // position of its single door, a wall cell other than a corner. Every
    // building stands wholly on its town, with a ring of open ground
    // around it, so the door always opens onto walkable ground.
    std::vector<SBuilding> buildings;

    // The difficulty of every cell: 0 in the town, 9 in the hardest
    // region. Opening the map up afterwards (see WildernessOpenings)
    // does not change it.
    CMatrix<int> difficulty;
};

} // export namespace RL

namespace RL
{

namespace voronoiwilderness_detail
{

// The terrain a region is made of
enum class Ter {
    Water, Grass, Forest, Field, Hills, Mountain, Snow, Town, Blocked
};

// Lakes and chasms are the only terrains no cell of a region can be
// walked on; every other region leads somewhere.
constexpr bool IsPassable(const Ter terrain)
{
    return terrain != Ter::Water && terrain != Ter::Blocked;
}

// A seed point of the diagram, in cell coordinates
struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

struct Region {
    // The seed point the region is the set of cells nearest to
    Vec2 center;

    // How many steps from the start region the flood fill got, and the
    // difficulty that was normalised out of it
    int bfs = -1;
    int difficulty = 0;

    Ter terrain = Ter::Grass;

    // How far the deepest cell of the region is from its edge, and the
    // gate cell of a Field region
    int max_edge = 1;
    int gate = -1;

    // Small buildings wholly within the town region, with open ground
    // around each footprint. Large regions get more, never larger, houses.
    std::vector<SBuilding> buildings;
};

// The tile types below are not among the library's builtin ones. They are
// registered on the first call, and only while the name is still free, so
// that a type registered under the same name earlier is kept as it is.
void RegisterWildernessTiles()
{
    const auto add = [](const std::string_view name, const STileData & data) {
        if (!CTileData::has(name)) {
            CTileData::RegisterTile(name, data);
        }
    };

    add("field", {U',', 0xD9C463, true, true});     // farmland
    add("hedge", {U'#', 0x2E7D32, false, false});   // what walls a field in
    add("slope", {CTileData::get("grass").second.glyph, 0xA7CC7C, true, true}); // elevation-tinted grass
    add("scrub", {U'&', 0x84945A, false, true});   // low hillside bushes
    add("heather", {U'*', 0xB889B4, true, true}); // sparse flowering ground cover
    add("animal_trail", {U'.', 0x929B70, true, true}); // faint worn ground
    add("fallen_log", {U'=', 0x8B6B43, true, true}); // low enough to step over
    add("fallen_log_vertical", {U'|', 0x8B6B43, true, true});
    add("peak", {U'*', 0xF5F5F5, false, false});    // snow-capped summit
    add("chasm", {U'%', 0x444444, true, false});    // too deep to cross
}

// Deterministic per-cell hash noise in [0, 1). The cheap value-noise base
// that everything fractal below is grown from.
double HashNoise(const int x, const int y, const std::uint64_t seed)
{
    std::uint64_t hash = seed;
    hash ^= static_cast<std::uint64_t>(x) * 0x9E3779B97F4A7C15ULL;
    hash = std::rotl(hash, 31);
    hash ^= static_cast<std::uint64_t>(y) * 0xC2B2AE3D27D4EB4FULL;
    hash *= 0xFF51AFD7ED558CCDULL;
    hash = std::rotl(hash, 29);

    return static_cast<double>(hash >> 11) * (1.0 / 9007199254740992.0);
}

// Smooth 2D value noise in [0, 1)
double SmoothNoise(const double x, const double y, const std::uint64_t seed)
{
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const double fx = x - x0;
    const double fy = y - y0;
    const double sx = fx * fx * (3.0 - 2.0 * fx);
    const double sy = fy * fy * (3.0 - 2.0 * fy);
    const double n00 = HashNoise(x0, y0, seed);
    const double n10 = HashNoise(x0 + 1, y0, seed);
    const double n01 = HashNoise(x0, y0 + 1, seed);
    const double n11 = HashNoise(x0 + 1, y0 + 1, seed);
    const double nx0 = n00 + (n10 - n00) * sx;
    const double nx1 = n01 + (n11 - n01) * sx;

    return nx0 + (nx1 - nx0) * sy;
}

// Fractal Brownian motion over the value noise, in [0, 1]
double Fbm(double x, double y, const std::uint64_t seed, const int octaves = 4)
{
    double sum = 0.0;
    double amplitude = 0.5;
    double total = 0.0;

    for (int octave = 0; octave < octaves; ++octave) {
        sum += amplitude * SmoothNoise(x, y, seed + static_cast<std::uint64_t>(octave) * 1013);
        total += amplitude;
        x *= 2.0;
        y *= 2.0;
        amplitude *= 0.5;
    }

    return sum / total;
}

struct Tri {
    int a;
    int b;
    int c;
};

// Whether p lies inside the circumcircle of the triangle abc. Solved for
// the circumcentre rather than for the radius squared, and the circle is
// shrunk by a hair so that a point landing exactly on an edge does not
// flip triangles unpredictably.
bool InCircumcircle(const Vec2 & a, const Vec2 & b, const Vec2 & c, const Vec2 & p)
{
    const double d = 2.0 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));

    if (std::abs(d) < 1e-12) {
        return false;   // degenerate: the three points are in a line
    }

    const double a2 = a.x * a.x + a.y * a.y;
    const double b2 = b.x * b.x + b.y * b.y;
    const double c2 = c.x * c.x + c.y * c.y;
    const double ux = (a2 * (b.y - c.y) + b2 * (c.y - a.y) + c2 * (a.y - b.y)) / d;
    const double uy = (a2 * (c.x - b.x) + b2 * (a.x - c.x) + c2 * (b.x - a.x)) / d;
    const double r2 = (a.x - ux) * (a.x - ux) + (a.y - uy) * (a.y - uy);
    const double dx = p.x - ux;
    const double dy = p.y - uy;

    return dx * dx + dy * dy < r2 * (1.0 - 1e-9);
}

// The Delaunay triangulation of the points (Bowyer-Watson), as index
// triples: start from a supertriangle around them all, then add the points
// one by one, tearing down every triangle the new point lies inside the
// circumcircle of and closing the hole up again from its bare edges.
std::vector<Tri> Delaunay(const std::vector<Vec2>& points)
{
    const int n = static_cast<int>(points.size());

    double minx = std::numeric_limits<double>::max();
    double miny = std::numeric_limits<double>::max();
    double maxx = std::numeric_limits<double>::lowest();
    double maxy = std::numeric_limits<double>::lowest();

    for (const Vec2 & point : points) {
        minx = std::min(minx, point.x);
        maxx = std::max(maxx, point.x);
        miny = std::min(miny, point.y);
        maxy = std::max(maxy, point.y);
    }

    const double span = std::max({maxx - minx, maxy - miny, 1.0});
    const double cx = (minx + maxx) / 2.0;
    const double cy = (miny + maxy) / 2.0;

    std::vector<Vec2> work = points;
    work.push_back({cx - 20 * span, cy - span});         // n
    work.push_back({cx, cy + 20 * span});                // n + 1
    work.push_back({cx + 20 * span, cy - span});         // n + 2

    std::vector<Tri> triangles;
    triangles.push_back({n, n + 1, n + 2});

    for (int i = 0; i < n; ++i) {
        // The edges of the cavity left by the triangles torn down
        std::map<std::pair<int, int>, int> edge_count;
        std::vector<Tri> keep;

        for (const Tri & tri : triangles) {
            if (InCircumcircle(work[tri.a], work[tri.b], work[tri.c], work[i])) {
                const std::array<std::pair<int, int>, 3> edges = {{
                        {std::min(tri.a, tri.b), std::max(tri.a, tri.b)},
                        {std::min(tri.b, tri.c), std::max(tri.b, tri.c)},
                        {std::min(tri.c, tri.a), std::max(tri.c, tri.a)}
                    }
                };

                for (const auto & edge : edges) {
                    ++edge_count[edge];
                }
            } else {
                keep.push_back(tri);
            }
        }

        triangles = std::move(keep);

        for (const auto & [edge, count] : edge_count) {
            if (count == 1) {
                triangles.push_back({edge.first, edge.second, i});
            }
        }
    }

    std::vector<Tri> result;

    for (const Tri & tri : triangles) {
        if (tri.a < n && tri.b < n && tri.c < n) {
            result.push_back(tri);
        }
    }

    return result;
}

// The edges of the triangulation: which region borders on which
std::set<std::pair<int, int>> AdjacencyFrom(const std::vector<Tri>& triangles)
{
    std::set<std::pair<int, int>> adjacency;

    for (const Tri & tri : triangles) {
        adjacency.insert({std::min(tri.a, tri.b), std::max(tri.a, tri.b)});
        adjacency.insert({std::min(tri.b, tri.c), std::max(tri.b, tri.c)});
        adjacency.insert({std::min(tri.c, tri.a), std::max(tri.c, tri.a)});
    }

    return adjacency;
}

// Whether the graph of passable regions is still connected once `blocked`
// is taken out of it - the test that keeps a lake or a chasm from cutting
// the wilderness in half.
bool PassableGraphConnected(const std::vector<Region>& regions,
                            const std::vector<std::vector<int>>& adjacency,
                            const int blocked)
{
    int start = -1;

    for (std::size_t i = 0; i < regions.size(); ++i) {
        if (static_cast<int>(i) != blocked && IsPassable(regions[i].terrain)) {
            start = static_cast<int>(i);
            break;
        }
    }

    if (start < 0) {
        return false;
    }

    std::vector<char> seen(regions.size(), 0);
    std::queue<int> queue;
    queue.push(start);
    seen[start] = 1;

    while (!queue.empty()) {
        const int from = queue.front();
        queue.pop();

        for (const int to : adjacency[from]) {
            if (to != blocked && !seen[to] && IsPassable(regions[to].terrain)) {
                seen[to] = 1;
                queue.push(to);
            }
        }
    }

    for (std::size_t i = 0; i < regions.size(); ++i) {
        if (static_cast<int>(i) != blocked && IsPassable(regions[i].terrain) && !seen[i]) {
            return false;
        }
    }

    return true;
}

// One wilderness: the article's pipeline, from scattered seed points to
// the terrain of every region. Everything but the tiles themselves, which
// Apply() writes onto a map.
class CWilderness
{
public:
    CWilderness(const int width, const int height, const int num_regions, const bool with_buildings);

    // Fill the map, and report what it was divided into
    [[nodiscard]] SWildernessInfo Apply(CMap & level) const;

private:
    int width_;
    int height_;
    bool with_buildings_;

    // The noise the point terrain of the open regions is cut from
    std::uint64_t noise_seed_ = 0;

    std::vector<Region> regions_;
    std::vector<int> region_id_;        // which region each cell belongs to
    std::vector<int> edge_dist_;        // how far each cell is from its region's edge, in thirds of a cell
    std::vector<double> elevation_;     // that depth normalised to 0..1 and smoothed
    std::vector<std::vector<int>> touching_;   // which regions share a border on the map
    std::vector<std::vector<int>> adjacency_;
    std::set<std::pair<int, int>> adjacency_set_;

    [[nodiscard]] int Index(const int x, const int y) const
    {
        return y * width_ + x;
    }

    // One chamfer relaxation step; (dx, dy) is the direction of the pass
    void Relax(const int x, const int y, const int dx, const int dy);

    // Terrain from difficulty: the article's "law" axis
    void AssignTerrain();

    // Gates for the fields, and small buildings for the town. Buildings
    // and their clearance must lie wholly within the town region.
    void PlaceFeatures();
    void PlaceTown(Region & region, std::size_t id);

    // How deep into its region a cell is: 0 at the edge, 1 at the deepest
    [[nodiscard]] double HeightAt(const int x, const int y) const;

    // The rule that merges two fields: where the neighbour has the lower
    // difficulty, no hedge is built on the shared edge
    [[nodiscard]] bool LowerFieldNeighbour(const int x, const int y, const Region & region) const;

    // What stands at one cell
    [[nodiscard]] std::string_view TileAt(const int x, const int y) const;
    void AddHillFeatures(CMap& level) const;
};

CWilderness::CWilderness(const int width, const int height, const int num_regions, const bool with_buildings)
    : width_(width), height_(height), with_buildings_(with_buildings)
{
    // One noise field for the whole map, drawn from the library's random
    // stream, so that the same seed gives the same wilderness again
    noise_seed_ = Random(0x7fffffff);

    // 1. Scatter the seed points of the diagram over the map
    regions_.resize(static_cast<std::size_t>(num_regions));

    for (Region & region : regions_) {
        region.center = {RandomFloat(0.0f, static_cast<float>(width_)),
                         RandomFloat(0.0f, static_cast<float>(height_))
                        };
    }

    // 2. Every cell belongs to the region of the point nearest to it: a
    // rasterised Voronoi diagram
    const std::size_t cells = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    region_id_.resize(cells);
    constexpr double warp = 6.0;
    constexpr double warp_scale = 0.07;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            double best = std::numeric_limits<double>::max();
            int best_id = 0;

            // Looked up at a point the noise pushes off the cell, so that
            // the borders between regions wander rather than run straight
            const double px = x + 0.5 + warp * (2.0 * Fbm(x * warp_scale, y * warp_scale, noise_seed_ + 7) - 1.0);
            const double py = y + 0.5 + warp * (2.0 * Fbm(x * warp_scale, y * warp_scale, noise_seed_ + 13) - 1.0);

            for (std::size_t i = 0; i < regions_.size(); ++i) {
                const double dx = regions_[i].center.x - px;
                const double dy = regions_[i].center.y - py;
                const double distance = dx * dx + dy * dy;

                if (distance < best) {
                    best = distance;
                    best_id = static_cast<int>(i);
                }
            }

            region_id_[Index(x, y)] = best_id;
        }
    }

    // How far every cell is from the edge of its region (a 3-4 chamfer
    // transform, in thirds of a cell). Cells with a value of 0 lie on a
    // region boundary, larger values are deeper inside.
    constexpr int infinity = std::numeric_limits<int>::max() / 4;
    edge_dist_.assign(cells, infinity);

    const auto boundary = [&](const int x, const int y) {
        const int id = region_id_[Index(x, y)];

        if (x == 0 || y == 0 || x == width_ - 1 || y == height_ - 1) {
            return true;
        }

        return region_id_[Index(x - 1, y)] != id || region_id_[Index(x + 1, y)] != id ||
               region_id_[Index(x, y - 1)] != id || region_id_[Index(x, y + 1)] != id;
    };

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (boundary(x, y)) {
                edge_dist_[Index(x, y)] = 0;
            }
        }
    }

    for (int y = 0; y < height_; ++y) {             // forward pass
        for (int x = 0; x < width_; ++x) {
            Relax(x, y, +1, +1);
        }
    }

    for (int y = height_ - 1; y >= 0; --y) {        // backward pass
        for (int x = width_ - 1; x >= 0; --x) {
            Relax(x, y, -1, -1);
        }
    }

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            Region & region = regions_[region_id_[Index(x, y)]];
            region.max_edge = std::max(region.max_edge, edge_dist_[Index(x, y)]);
        }
    }

    // The depth as a height between 0 at the edge and 1 at the deepest
    // cell, blurred within the region: contours of the distance to a
    // polygon's edge keep the polygon's corners, blurring rounds them off
    elevation_.resize(cells);

    for (std::size_t cell = 0; cell < cells; ++cell) {
        elevation_[cell] = static_cast<double>(edge_dist_[cell]) / std::max(1, regions_[region_id_[cell]].max_edge);
    }

    for (int pass = 0; pass < 3; ++pass) {
        std::vector<double> blurred(cells);

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const int id = region_id_[Index(x, y)];
                double sum = 0.0;
                int count = 0;

                for (int ny = std::max(0, y - 1); ny <= std::min(height_ - 1, y + 1); ++ny) {
                    for (int nx = std::max(0, x - 1); nx <= std::min(width_ - 1, x + 1); ++nx) {
                        if (region_id_[Index(nx, ny)] == id) {
                            sum += elevation_[Index(nx, ny)];
                            ++count;
                        }
                    }
                }

                blurred[Index(x, y)] = sum / count;
            }
        }

        elevation_ = std::move(blurred);
    }

    // Which regions really share a border on the map
    touching_.assign(regions_.size(), {});

    const auto touch = [&](const int a, const int b) {
        if (a != b && std::ranges::find(touching_[a], b) == touching_[a].end()) {
            touching_[a].push_back(b);
            touching_[b].push_back(a);
        }
    };

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (x + 1 < width_) {
                touch(region_id_[Index(x, y)], region_id_[Index(x + 1, y)]);
            }

            if (y + 1 < height_) {
                touch(region_id_[Index(x, y)], region_id_[Index(x, y + 1)]);
            }
        }
    }

    // 3. The Delaunay triangulation of the seed points: the adjacency
    // graph the regions are flood filled over
    std::vector<Vec2> points;
    points.reserve(regions_.size());

    for (const Region & region : regions_) {
        points.push_back(region.center);
    }

    adjacency_set_ = AdjacencyFrom(Delaunay(points));
    adjacency_.assign(regions_.size(), {});

    for (const auto & [a, b] : adjacency_set_) {
        adjacency_[a].push_back(b);
        adjacency_[b].push_back(a);
    }

    // 4. Flood fill the difficulties from a random vertex, then normalise
    // them to the 0 to 9 the terrain is picked from. The start becomes the
    // town, so it is picked among the regions deep enough to hold the
    // smallest building with open ground around it; failing that, among
    // those that own a cell of the map at all.
    constexpr int town_depth = 15;    // five cells, in thirds of a cell
    std::vector<std::size_t> owned(regions_.size(), 0);

    for (const int id : region_id_) {
        ++owned[id];
    }

    std::vector<std::size_t> candidates;

    constexpr std::array<int, 2> depths = {town_depth, 0};

    for (const int depth : depths) {
        for (std::size_t i = 0; i < regions_.size(); ++i) {
            if (owned[i] > 0 && regions_[i].max_edge >= depth) {
                candidates.push_back(i);
            }
        }

        if (!candidates.empty()) {
            break;
        }
    }

    const std::size_t start = candidates.empty() ? Random(regions_.size()) : candidates[Random(candidates.size())];
    std::queue<int> queue;
    queue.push(static_cast<int>(start));
    regions_[start].bfs = 0;
    int max_bfs = 0;

    while (!queue.empty()) {
        const int from = queue.front();
        queue.pop();
        max_bfs = std::max(max_bfs, regions_[from].bfs);

        for (const int to : adjacency_[from]) {
            if (regions_[to].bfs < 0) {
                regions_[to].bfs = regions_[from].bfs + 1;
                queue.push(to);
            }
        }
    }

    constexpr int max_difficulty = 9;

    for (Region & region : regions_) {
        // A region the fill could not reach - only possible in a
        // degenerate triangulation - is as hard as it gets
        region.difficulty = region.bfs < 0 ? max_difficulty :
                            static_cast<int>(std::lround(static_cast<double>(region.bfs) * max_difficulty /
                                                         std::max(1, max_bfs)));
    }

    AssignTerrain();
    PlaceFeatures();
}

void CWilderness::Relax(const int x, const int y, const int dx, const int dy)
{
    const int here = Index(x, y);

    // A straight step costs 3, a diagonal one 4: close enough to the 3 and
    // 4.24 of the true distance that contours come out round, not square
    const auto better = [&](const int nx, const int ny, const int cost) {
        if (nx < 0 || ny < 0 || nx >= width_ || ny >= height_) {
            return;
        }

        const int there = Index(nx, ny);

        if (edge_dist_[there] + cost < edge_dist_[here]) {
            edge_dist_[here] = edge_dist_[there] + cost;
        }
    };

    better(x - dx, y, 3);
    better(x, y - dy, 3);
    better(x - dx, y - dy, 4);
    better(x + dx, y - dy, 4);
}

void CWilderness::AssignTerrain()
{
    // The difficulty-0 region the flood fill started in is the town
    std::size_t town = 0;

    for (std::size_t i = 0; i < regions_.size(); ++i) {
        if (regions_[i].difficulty == 0) {
            town = i;
            break;
        }
    }

    regions_[town].terrain = Ter::Town;

    std::vector<std::size_t> order(regions_.size());
    std::iota(order.begin(), order.end(), 0);
    Shuffle(order.begin(), order.end());

    // The base terrain, by difficulty
    for (const std::size_t i : order) {
        if (i == town) {
            continue;
        }

        const int difficulty = regions_[i].difficulty;
        const float roll = RandomFloat();

        if (difficulty <= 1) {
            regions_[i].terrain = roll < 0.45f ? Ter::Field : Ter::Grass;
        } else if (difficulty <= 3) {
            regions_[i].terrain = roll < 0.40f ? Ter::Grass : roll < 0.75f ? Ter::Forest : Ter::Field;
        } else if (difficulty <= 5) {
            regions_[i].terrain = roll < 0.50f ? Ter::Forest : roll < 0.85f ? Ter::Hills : Ter::Grass;
        } else if (difficulty <= 7) {
            regions_[i].terrain = roll < 0.55f ? Ter::Hills : roll < 0.90f ? Ter::Mountain : Ter::Forest;
        } else {
            regions_[i].terrain = roll < 0.65f ? Ter::Mountain : Ter::Snow;
        }
    }

    // Lakes and chasms, but only where the passable graph stays connected
    const int want_water = std::max(1, static_cast<int>(regions_.size()) / 12);
    const int want_blocked = std::max(1, static_cast<int>(regions_.size()) / 14);
    int have_water = 0;
    int have_blocked = 0;

    for (const std::size_t i : order) {
        if (i == town) {
            continue;
        }

        if (have_water < want_water && regions_[i].difficulty >= 1 &&
                regions_[i].terrain != Ter::Snow &&
                PassableGraphConnected(regions_, touching_, static_cast<int>(i))) {
            regions_[i].terrain = Ter::Water;
            ++have_water;
            continue;
        }

        if (have_blocked < want_blocked && regions_[i].difficulty >= 3 &&
                PassableGraphConnected(regions_, touching_, static_cast<int>(i))) {
            regions_[i].terrain = Ter::Blocked;
            ++have_blocked;
        }
    }
}

void CWilderness::PlaceFeatures()
{
    for (std::size_t i = 0; i < regions_.size(); ++i) {
        Region & region = regions_[i];

        if (region.terrain == Ter::Field) {
            // A gate on a random boundary cell
            std::vector<int> borders;

            for (int y = 0; y < height_; ++y) {
                for (int x = 0; x < width_; ++x) {
                    const int cell = Index(x, y);

                    if (region_id_[cell] == static_cast<int>(i) && edge_dist_[cell] == 0) {
                        borders.push_back(cell);
                    }
                }
            }

            if (!borders.empty()) {
                region.gate = borders[Random(borders.size())];
            }
        } else if (region.terrain == Ter::Town && with_buildings_) {
            PlaceTown(region, i);
        }
    }
}

void CWilderness::PlaceTown(Region& region, std::size_t id)
{
    auto town = [&](int x, int y) {
        return x >= 0 && y >= 0 && x < width_ && y < height_ &&
               region_id_[Index(x, y)] == static_cast<int>(id);
    };
    std::vector<bool> occupied(width_ * height_);
    auto fits = [&](int left, int top, int width, int height) {
        for (int y = top - 1; y <= top + height; ++y) {
            for (int x = left - 1; x <= left + width; ++x) {
                if (!town(x, y) || occupied[Index(x, y)]) {
                    return false;
                }
            }
        }
        return true;
    };
    std::vector<Position> spots;
    for (int y = 1; y + 4 < height_; ++y) {
        for (int x = 1; x + 4 < width_; ++x) {
            if (fits(x, y, 4, 4)) {
                spots.emplace_back(x, y);
            }
        }
    }
    Shuffle(spots.begin(), spots.end());
    for (auto spot : spots) {
        const int left = spot.x, top = spot.y;
        int width = 4 + Random(4), height = 4 + Random(4);
        if (!fits(left, top, width, height)) {
            width = 4;
            height = 4;
            if (!fits(left, top, width, height)) {
                continue;
            }
        }
        SBuilding building;
        building.corner1 = spot;
        building.corner2 = Position(left + width - 1, top + height - 1);
        std::vector<Position> doors;
        for (int x = left + 1; x < left + width - 1; ++x) {
            doors.emplace_back(x, top);
            doors.emplace_back(x, top + height - 1);
        }
        for (int y = top + 1; y < top + height - 1; ++y) {
            doors.emplace_back(left, y);
            doors.emplace_back(left + width - 1, y);
        }
        building.door = doors[Random(doors.size())];
        region.buildings.push_back(building);
        for (int y = top; y < top + height; ++y) {
            for (int x = left; x < left + width; ++x) {
                occupied[Index(x, y)] = true;
            }
        }
    }
}

double CWilderness::HeightAt(const int x, const int y) const
{
    return elevation_[Index(x, y)];
}

bool CWilderness::LowerFieldNeighbour(const int x, const int y, const Region & region) const
{
    const int id = region_id_[Index(x, y)];

    const auto lower = [&](const int nx, const int ny) {
        if (nx < 0 || ny < 0 || nx >= width_ || ny >= height_) {
            return false;
        }

        const int neighbour = region_id_[Index(nx, ny)];

        return neighbour != id && regions_[neighbour].terrain == Ter::Field &&
               regions_[neighbour].difficulty < region.difficulty;
    };

    return lower(x - 1, y) || lower(x + 1, y) || lower(x, y - 1) || lower(x, y + 1);
}

std::string_view CWilderness::TileAt(const int x, const int y) const
{
    const int cell = Index(x, y);
    const Region & region = regions_[region_id_[cell]];
    const double noise = Fbm(x * 0.25, y * 0.25, noise_seed_);
    const int distance = edge_dist_[cell];

    switch (region.terrain) {
    case Ter::Water:
        return "water";

    case Ter::Blocked:
        return "chasm";

    case Ter::Grass:            // open region: sparse point terrain
        if (noise > 0.88) {
            return "tree";
        }

        if (noise < 0.06) {
            return "plant";
        }

        return "grass";

    case Ter::Forest: {
        // Match GenerateForest's lighter mix: about 60% vegetation, of
        // which 70% is trees, with 5% scattered rocks. Slow density noise
        // creates thickets and thinner patches; independent cell hashes
        // scatter individual plants instead of filling solid tree blobs.
        const double vegetation = 0.45 + 0.30 * Fbm(x * 0.07, y * 0.07, noise_seed_ + 131);
        const double scatter = HashNoise(x, y, noise_seed_ + 137);
        if (scatter < 0.05) {
            return "rock";
        }
        if (scatter < 0.05 + 0.95 * vegetation) {
            return HashNoise(x, y, noise_seed_ + 149) < 0.70 ? "tree" : "plant";
        }
        return "grass";
    }

    case Ter::Field:

        // An impassable hedge around the field with a single gate; where a
        // neighbouring field has the lower difficulty, no hedge is built on
        // the shared edge (the merging rule of the article)
        if (distance == 0) {
            if (cell == region.gate) {
                return "door_open";
            }

            return LowerFieldNeighbour(x, y, region) ? "field" : "hedge";
        }

        return noise > 0.90 ? "plant" : "field";

    case Ter::Hills: {
        // Sparse rocks do not turn the hilltops into continuous barriers.
        // Scrub and flowering heather favour coherent vegetation patches.
        const double scatter = HashNoise(x, y, noise_seed_ + 163);
        const double cover = Fbm(x * 0.10, y * 0.10, noise_seed_ + 167);
        if (scatter < 0.02) {
            return "rock";
        }
        if (scatter < 0.04 && cover > 0.55) {
            return "heather";
        }
        if (scatter < 0.10 + 0.10 * cover) {
            return "scrub";
        }
        return "slope";
    }

    case Ter::Mountain: {
        // The smoothed height, its outline worn by the noise; past the
        // rock line the mountain is impassable
        const double height = HeightAt(x, y) + 0.4 * (noise - 0.5);

        if (height > 0.45) {
            return "wall";
        }

        if (height > 0.25) {
            return "slope";
        }

        return "grass";
    }

    case Ter::Snow: {
        const double height = HeightAt(x, y) + 0.4 * (noise - 0.5);

        if (height > 0.50) {
            return "peak";
        }

        if (height > 0.32) {
            return "rock";
        }

        return "slope";
    }

    case Ter::Town:

        // Each building and its clearance lie wholly within the town.
        for (const auto& building : region.buildings) {
            const int left = building.corner1.x, top = building.corner1.y;
            const int right = building.corner2.x, bottom = building.corner2.y;
            if (x < left || x > right || y < top || y > bottom) {
                continue;
            }
            if (Position(x, y) == building.door) {
                return "door_open";
            }
            return x == left || x == right || y == top || y == bottom ? "wall" : "room";
        }

        return "grass";
    }

    return "wall";
}

// Small trails wander through each hillside, following its contours and
// staying within the region. Logs are sparse short objects, not scatter
// noise; neither feature blocks movement or replaces other biomes.
void CWilderness::AddHillFeatures(CMap& level) const
{
    std::vector<std::vector<int>> cells(regions_.size());
    for (int cell = 0; cell < width_ * height_; ++cell) {
        const auto id = region_id_[cell];
        if (regions_[id].terrain == Ter::Hills) {
            cells[id].push_back(cell);
        }
    }
    auto ground = [&](int cell) {
        const auto type = level.get(cell % width_, cell / width_).getType();
        return type == "slope" || type == "scrub" || type == "heather";
    };
    constexpr std::array<std::pair<int, int>, 4> directions{{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
    std::vector<bool> visited(width_ * height_);
    for (std::size_t id = 0; id < cells.size(); ++id) {
        if (cells[id].size() < 12) {
            continue;
        }
        const auto seed = noise_seed_ + 179 + id * 1009;
        int start = -1;
        for (int cell : cells[id]) {
            if (ground(cell) && (start < 0 || edge_dist_[cell] > edge_dist_[start])) {
                start = cell;
            }
        }
        if (start < 0) {
            continue;
        }
        int heading = static_cast<int>(HashNoise(start, 0, seed) * 4);
        int current = start;
        std::vector<int> trail{start};
        visited[start] = true;
        // Only some larger hills have a short, faint trail. Most remain
        // undisturbed instead of drawing a path through every region.
        const bool has_trail = cells[id].size() >= 80 && HashNoise(start, 5, seed) < 0.30;
        const int length = has_trail ? std::clamp(static_cast<int>(0.75 * std::sqrt(cells[id].size())), 6, 16) : 0;
        for (int step = 1; step < length; ++step) {
            int best = -1, best_direction = heading;
            double lowest = std::numeric_limits<double>::max();
            for (int d = 0; d < 4; ++d) {
                if (d == (heading + 2) % 4) {
                    continue; // Never curl back into a tight zigzag.
                }
                const int x = current % width_ + directions[d].first;
                const int y = current / width_ + directions[d].second;
                if (x < 0 || y < 0 || x >= width_ || y >= height_) {
                    continue;
                }
                const int cell = Index(x, y);
                if (region_id_[cell] != static_cast<int>(id) || visited[cell] || !ground(cell)) {
                    continue;
                }
                const double turn = step % 4 == 0 ? 0.20 : 0.80;
                const double score = 0.45 * HashNoise(x, y, seed + step) + (d == heading ? 0 : turn) +
                                     2 * std::abs(elevation_[cell] - elevation_[current]);
                if (score < lowest) {
                    lowest = score;
                    best = cell;
                    best_direction = d;
                }
            }
            if (best < 0) {
                break;
            }
            current = best;
            heading = best_direction;
            visited[current] = true;
            trail.push_back(current);
        }
        if (trail.size() >= 6) {
            for (int cell : trail) {
                level.SetCell(cell % width_, cell / width_, "animal_trail");
            }
        }
        // About one short log per 300 hillside tiles. Candidate ordering
        // and orientation are seeded, so the same map remains reproducible.
        int remaining = std::max(1, static_cast<int>(cells[id].size() / 300));
        if (HashNoise(start, 1, seed) < 0.35) {
            continue; // Some hills are entirely free of fallen timber.
        }
        for (int cell : cells[id]) {
            if (remaining == 0) {
                break;
            }
            if (HashNoise(cell, 2, seed) > 0.03 || !ground(cell)) {
                continue;
            }
            const bool vertical = HashNoise(cell, 3, seed) < 0.5;
            const int size = HashNoise(cell, 4, seed) < 0.5 ? 2 : 3;
            std::vector<int> log;
            for (int offset = 0; offset < size; ++offset) {
                const int x = cell % width_ + (vertical ? 0 : offset);
                const int y = cell / width_ + (vertical ? offset : 0);
                if (x >= width_ || y >= height_) {
                    break;
                }
                const int next = Index(x, y);
                if (region_id_[next] != static_cast<int>(id) || !ground(next)) {
                    break;
                }
                log.push_back(next);
            }
            if (log.size() == static_cast<std::size_t>(size)) {
                for (int next : log) {
                    level.SetCell(next % width_, next / width_, vertical ? "fallen_log_vertical" : "fallen_log");
                }
                --remaining;
            }
        }
    }
}

SWildernessInfo CWilderness::Apply(CMap & level) const
{
    SWildernessInfo info;
    info.regions = regions_.size();
    info.adjacency = adjacency_set_.size();
    info.difficulty = CMatrix<int>(level.getSize(), 0);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const auto type = TileAt(x, y);
            if (type == "slope") {
                const auto terrain = regions_[region_id_[Index(x, y)]].terrain;
                const double elevation = terrain == Ter::Hills ?
                                         0.5 * HeightAt(x, y) + 0.5 * Fbm(x * 0.15, y * 0.15, noise_seed_ + 99) :
                                         HeightAt(x, y);
                const double blend = std::clamp(elevation, 0.0, 1.0);
                const auto low = CTileData::get("grass").second.rgb_color;
                constexpr std::uint32_t high = 0xA08A5B;
                std::uint32_t colour = 0;
                for (int shift : {16, 8, 0}) {
                    const double channel = ((low >> shift) & 0xFF) * (1.0 - blend) +
                                           ((high >> shift) & 0xFF) * blend;
                    colour |= static_cast<std::uint32_t>(std::lround(channel)) << shift;
                }
                auto data = CTileData::get(type).second;
                data.rgb_color = GetJitteredColor(colour);
                level.get(x, y) = CTile(type, data);
            } else {
                level.SetCell(static_cast<std::size_t>(x), static_cast<std::size_t>(y), type);
            }
            info.difficulty.set(static_cast<std::size_t>(x), static_cast<std::size_t>(y),
                                regions_[region_id_[Index(x, y)]].difficulty);
        }
    }

    AddHillFeatures(level);

    for (std::size_t i = 0; i < regions_.size(); ++i) {
        const Region & region = regions_[i];

        if (region.terrain != Ter::Town) {
            continue;
        }

        // The town is where its region lies deepest
        int deepest = -1;

        for (std::size_t cell = 0; cell < region_id_.size(); ++cell) {
            if (region_id_[cell] == static_cast<int>(i) && (deepest < 0 || edge_dist_[cell] > edge_dist_[deepest])) {
                deepest = static_cast<int>(cell);
            }
        }

        if (deepest >= 0) {
            info.locations.push_back({ELocationType::Town,
                                      Position(static_cast<std::size_t>(deepest % width_),
                                               static_cast<std::size_t>(deepest / width_)),
                                      static_cast<std::size_t>(std::max(1, edge_dist_[deepest] / 3))
                                     });
        }

        info.buildings.insert(info.buildings.end(), region.buildings.begin(), region.buildings.end());
    }

    return info;
}

} // namespace voronoiwilderness_detail

} // namespace RL

export namespace RL
{

// What may be opened up to make the wilderness walkable everywhere: a
// hedge is broken where a field's gate does not lead out, a tree is felled
// to get through a forest, and a lake is bridged rather than steered
// around when it is in the way. Mountains, peaks and chasms are never
// opened. See MakeReachable() in rl.connectivity.
inline constexpr std::array<SOpening, 3> WildernessOpenings = {{
        {"hedge", 3, "field"},
        {"tree", 4, "grass"},
        {"water", 5, "bridge"}
    }
};

// Generate a wilderness over the whole map: seed points scattered at
// random, every cell in the region of the point nearest to it, the
// difficulties by flood fill over the Delaunay adjacency of the regions,
// and the terrain of each region picked from its difficulty. The town sits
// at difficulty 0, the hardest peaks at 9, and lakes and chasms go in only
// where the passable regions stay connected.
//
// The tile types field, hedge, slope, peak and chasm are registered the
// first time this runs; a game that registered a type of one of those
// names itself keeps its own.
//
// @param level           The map to fill. An empty one is left alone.
// @param num_regions     How many Voronoi regions to scatter, at least 4.
// At 0 one region is asked for per 200 cells of the
// map, with at least 10. Larger maps add regions instead of enlarging them.
// @param with_buildings  Whether to fill the town with small buildings.
// Each side, including walls, is 4–7 tiles long; larger towns get more
// buildings. Narrow plots that cannot hold a 4x4 building stay open.
//
// @return The regions and their adjacency edges, the locations the
// generator laid out - the starting town, as an SLocation of type
// Town - the buildings, and the difficulty of every cell.
SWildernessInfo CreateVoronoiWilderness(CMap & level, int num_regions = 0, bool with_buildings = false)
{
    const int width = static_cast<int>(level.getWidth());
    const int height = static_cast<int>(level.getHeight());

    if (width == 0 || height == 0) {
        return SWildernessInfo();
    }

    voronoiwilderness_detail::RegisterWildernessTiles();

    if (num_regions <= 0) {
        num_regions = std::max(width * height / 200, 10);
    } else {
        num_regions = std::clamp(num_regions, 4, 800);
    }

    const voronoiwilderness_detail::CWilderness wilderness(width, height, num_regions, with_buildings);
    SWildernessInfo info = wilderness.Apply(level);

    // Whatever the connectivity of the regions guaranteed, a gate may
    // still lead into a hedge and a forest may still be too dense to
    // cross. Open up whatever pockets are left, the cheapest way first.
    MakeReachable(level, WildernessOpenings);

    return info;
}

} // export namespace RL

module;

//////////////////////////////////////////////////////////////////////////
// Multi-Region Wilderness (formerly Voronoi Wilderness)
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

export module rl.mapgenerators.multiregion;

export import rl.connectivity;
export import rl.locations;
export import rl.map;
export import rl.mapgenerators.biomes;
import rl.matrix;
import rl.maputils;
import rl.position;
import rl.randomness;
import rl.tile;
import std;

export namespace RL
{

// What the generator worked out, next to the tiles it set.
struct SMultiRegionInfo {
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

    // Field entrances, facing the village and linked to its road network.
    std::vector<Position> field_gates;

    // Cultivable cells of each field region, excluding hedges, gates and roads.
    std::vector<std::vector<Position>> fields;

    // The open grassland of each grass region: its walkable cells, as they
    // are once the map is opened up and the roads laid, excluding the roads -
    // a meadow, irregular as its region, for grazing, say.
    std::vector<std::vector<Position>> meadows;

    // The difficulty of every cell: 0 in the town, 9 in the hardest
    // region. Opening the map up afterwards (see WildernessOpenings)
    // does not change it.
    CMatrix<int> difficulty;
};

// Configuration options for MultiRegion wilderness generation, including customizable biomes.
struct SMultiRegionConfig {
    int num_regions = 0;              ///< Number of regions (0 = automatic based on map area)
    bool with_buildings = false;      ///< True to generate town buildings; false for uncultivated bogs
    TerrainBiome forest_biome = SForestBiome{};     ///< Biome pattern used for forest regions
    TerrainBiome bog_biome = SBogBiome{};           ///< Biome pattern used for bog regions
    TerrainBiome hills_biome = SHillsBiome{};       ///< Biome pattern used for rolling hills
    TerrainBiome grass_biome = SGrasslandBiome{};   ///< Biome pattern used for open grassland
};

// Backward-compatibility aliases
using SWildernessInfo = SMultiRegionInfo;
using SWildernessConfig = SMultiRegionConfig;

} // export namespace RL

namespace RL
{

namespace voronoiwilderness_detail
{

// The terrain a region is made of
enum class Ter {
    Water, Grass, Forest, Field, Bog, Hills, Mountain, Snow, Town, Blocked
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
    RegisterBiomeTiles();
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
    CWilderness(const int width, const int height, const SWildernessConfig& config);

    // Fill the map, and report what it was divided into
    [[nodiscard]] SWildernessInfo Apply(CMap & level) const;
    void ConnectTownDoors(CMap& level) const;
    void ConnectFieldRoads(CMap& level) const;
    [[nodiscard]] std::vector<std::vector<Position>> Fields(const CMap& level) const;
    [[nodiscard]] std::vector<std::vector<Position>> Meadows(const CMap& level) const;

private:
    int width_;
    int height_;
    bool with_buildings_;
    SWildernessConfig config_;

    // The noise the point terrain of the open regions is cut from
    std::uint64_t noise_seed_ = 0;

    std::vector<Region> regions_;
    std::vector<int> region_id_;        // which region each cell belongs to
    std::vector<int> edge_dist_;        // how far each cell is from its region's edge, in thirds of a cell
    std::vector<int> chasm_depth_;     // distance from the irregular ravine rim
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
    void ShapeChasms();
};

CWilderness::CWilderness(const int width, const int height, const SWildernessConfig& config)
    : width_(width), height_(height), with_buildings_(config.with_buildings), config_(config)
{
    // One noise field for the whole map, drawn from the library's random
    // stream, so that the same seed gives the same wilderness again
    noise_seed_ = Random(0x7fffffff);

    // 1. Scatter the seed points of the diagram over the map
    regions_.resize(static_cast<std::size_t>(config.num_regions));

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
    ShapeChasms();
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

    // Unsettled wilderness has wet lowlands in place of cultivated fields.
    if (!with_buildings_) {
        for (auto& region : regions_) {
            if (region.terrain == Ter::Field) region.terrain = Ter::Bog;
        }
    }
}

// Carve a branching ravine inside each selected region rather than using
// the whole Voronoi polygon as a uniformly blocked patch. Leave its outer
// shoulder open, so the new shape adds no barriers to neighbouring regions.
void CWilderness::ShapeChasms()
{
    std::vector<std::vector<int>> cells(regions_.size());
    for (int cell = 0; cell < width_ * height_; ++cell) {
        if (regions_[region_id_[cell]].terrain == Ter::Blocked) {
            cells[region_id_[cell]].push_back(cell);
        }
    }
    std::vector<bool> mask(width_ * height_);
    for (std::size_t id = 0; id < cells.size(); ++id) {
        if (cells[id].empty()) {
            continue;
        }
        int centre = cells[id].front();
        int x0 = width_, x1 = 0, y0 = height_, y1 = 0;
        for (int cell : cells[id]) {
            if (edge_dist_[cell] > edge_dist_[centre]) { centre = cell; }
            x0 = std::min(x0, cell % width_);
            x1 = std::max(x1, cell % width_);
            y0 = std::min(y0, cell / width_);
            y1 = std::max(y1, cell / width_);
        }
        const auto seed = noise_seed_ + 191 + id * 1013;
        const double cx = centre % width_, cy = centre / width_;
        const double angle = (x1 - x0 >= y1 - y0 ? 0 : 1.570796326794897) +
                             (HashNoise(centre, 0, seed) - 0.5) * 0.60;
        auto paint = [&](double px, double py, double radius) {
            for (int y = std::max(0, static_cast<int>(std::floor(py - radius - 1)));
                    y <= std::min(height_ - 1, static_cast<int>(std::ceil(py + radius + 1))); ++y) {
                for (int x = std::max(0, static_cast<int>(std::floor(px - radius - 1)));
                        x <= std::min(width_ - 1, static_cast<int>(std::ceil(px + radius + 1))); ++x) {
                    const int cell = Index(x, y);
                    if (region_id_[cell] != static_cast<int>(id) || edge_dist_[cell] == 0) { continue; }
                    const double edge = radius + 0.30 * (Fbm(x * 0.35, y * 0.35, seed) - 0.5);
                    if ((x - px) * (x - px) + (y - py) * (y - py) <= edge * edge) {
                        mask[cell] = true;
                    }
                }
            }
        };
        auto arm = [&](double direction, double length, int branch) {
            const double bend = (HashNoise(centre, branch, seed) - 0.5) * length * 0.30;
            const int steps = std::max(1, static_cast<int>(std::ceil(length * 2)));
            for (int step = 0; step <= steps; ++step) {
                const double t = static_cast<double>(step) / steps;
                const double curve = std::sin(t * 3.141592653589793) * bend;
                const double x = cx + std::cos(direction) * length * t - std::sin(direction) * curve;
                const double y = cy + std::sin(direction) * length * t + std::cos(direction) * curve;
                const int ix = static_cast<int>(std::lround(x)), iy = static_cast<int>(std::lround(y));
                if (ix < 0 || iy < 0 || ix >= width_ || iy >= height_ ||
                        region_id_[Index(ix, iy)] != static_cast<int>(id)) { break; }
                const double radius = 1.05 + (1.0 - t) * 1.35 +
                                      0.45 * Fbm(x * 0.22, y * 0.22, seed + branch);
                paint(x, y, radius);
            }
        };
        const double span = std::max(x1 - x0, y1 - y0);
        arm(angle, span * 0.55, 1);
        arm(angle + 3.141592653589793, span * 0.55, 2);
        arm(angle + (HashNoise(centre, 3, seed) < 0.5 ? 1 : -1) * 1.15, span * 0.35, 4);
    }
    // Eight-neighbour distance shading follows the actual fissure edges,
    // including bends and branches, rather than the original region shape.
    chasm_depth_.assign(width_ * height_, 0);
    std::queue<int> pending;
    for (int cell = 0; cell < width_ * height_; ++cell) {
        if (!mask[cell]) { continue; }
        bool rim = false;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int x = cell % width_ + dx, y = cell / width_ + dy;
                rim |= x < 0 || y < 0 || x >= width_ || y >= height_ || !mask[Index(x, y)];
            }
        }
        if (rim) {
            chasm_depth_[cell] = 1;
            pending.push(cell);
        }
    }
    while (!pending.empty()) {
        const int cell = pending.front(); pending.pop();
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int x = cell % width_ + dx, y = cell / width_ + dy;
                if (x < 0 || y < 0 || x >= width_ || y >= height_) { continue; }
                const int next = Index(x, y);
                if (mask[next] && chasm_depth_[next] == 0) {
                    chasm_depth_[next] = chasm_depth_[cell] + 1;
                    pending.push(next);
                }
            }
        }
    }
}

void CWilderness::PlaceFeatures()
{
    for (std::size_t i = 0; i < regions_.size(); ++i) {
        Region & region = regions_[i];

        if (region.terrain == Ter::Field) {
            // Prefer a boundary opening on the side facing the village.
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
                const auto village = std::ranges::find_if(regions_, [](const auto& r) { return r.terrain == Ter::Town; });
                double best = std::numeric_limits<double>::max();
                for (int cell : borders) {
                    const int x = cell % width_, y = cell / width_;
                    bool faces = false, exit = false;
                    for (const auto [dx, dy] : std::array<std::pair<int, int>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
                        const int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= width_ || ny >= height_) { continue; }
                        const int next = region_id_[Index(nx, ny)];
                        if (next == static_cast<int>(i) || !IsPassable(regions_[next].terrain)) { continue; }
                        const auto outside = TileAt(nx, ny);
                        if (!CTileData::get(outside).second.passable && outside != "tree") { continue; }
                        exit = true;
                        faces |= dx * (village->center.x - region.center.x) + dy * (village->center.y - region.center.y) > 0;
                    }
                    if (!exit) { continue; }
                    const double dx = x - village->center.x, dy = y - village->center.y;
                    const double score = dx * dx + dy * dy + (faces ? 0 : width_ * width_ + static_cast<double>(height_) * height_);
                    if (score < best) { best = score; region.gate = cell; }
                }
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
    // Aim each entrance towards the closest neighbouring house. Choose
    // the facing wall first, then align the door along that wall.
    for (auto& building : region.buildings) {
        const double cx = (building.corner1.x + building.corner2.x) / 2.0;
        const double cy = (building.corner1.y + building.corner2.y) / 2.0;
        const SBuilding* neighbor = nullptr;
        double nearest = std::numeric_limits<double>::max();
        for (const auto& other : region.buildings) {
            if (&other == &building) { continue; }
            const double dx = (other.corner1.x + other.corner2.x) / 2.0 - cx;
            const double dy = (other.corner1.y + other.corner2.y) / 2.0 - cy;
            const double distance = dx * dx + dy * dy;
            if (distance < nearest) {
                nearest = distance;
                neighbor = &other;
            }
        }
        if (!neighbor) { continue; }
        const double tx = (neighbor->corner1.x + neighbor->corner2.x) / 2.0;
        const double ty = (neighbor->corner1.y + neighbor->corner2.y) / 2.0;
        if (std::abs(tx - cx) >= std::abs(ty - cy)) {
            building.door = Position(tx > cx ? building.corner2.x : building.corner1.x,
                                     std::clamp(static_cast<std::size_t>(std::lround(ty)), building.corner1.y + 1, building.corner2.y - 1));
        } else {
            building.door = Position(std::clamp(static_cast<std::size_t>(std::lround(tx)), building.corner1.x + 1, building.corner2.x - 1),
                                     ty > cy ? building.corner2.y : building.corner1.y);
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
        if (chasm_depth_[cell] == 0) {
            return "grass"; // Open shoulders around the ravine.
        }
        return chasm_depth_[cell] == 1 ? "chasm_rim" : chasm_depth_[cell] == 2 ? "chasm_wall" : "chasm";

    case Ter::Grass: {
        if (std::holds_alternative<SGrasslandBiome>(config_.grass_biome)) {
            const auto& gb = std::get<SGrasslandBiome>(config_.grass_biome);
            if (noise > gb.tree_threshold) return gb.tree_tile;
            if (noise < gb.plant_threshold) return gb.plant_tile;
            return gb.base_tile;
        }
        return "grass";
    }

    case Ter::Forest: {
        if (std::holds_alternative<SForestBiome>(config_.forest_biome)) {
            const auto& fb = std::get<SForestBiome>(config_.forest_biome);
            double vegetation = fb.vegetation_density;
            if (fb.patchiness > 0.0f) {
                const double fbm = Fbm(x * 0.07, y * 0.07, noise_seed_ + 131);
                vegetation = std::clamp(static_cast<double>(fb.vegetation_density) +
                                        static_cast<double>(fb.patchiness) * (fbm - 0.5), 0.0, 1.0);
            }
            const double scatter = HashNoise(x, y, noise_seed_ + 137);
            if (scatter < fb.rock_density) {
                return fb.rock_tile;
            }
            if (scatter < fb.rock_density + (1.0 - fb.rock_density) * vegetation) {
                return HashNoise(x, y, noise_seed_ + 149) < fb.tree_density ? fb.tree_tile : fb.plant_tile;
            }
            return fb.base_tile;
        }
        return "grass";
    }

    case Ter::Bog: {
        if (std::holds_alternative<SBogBiome>(config_.bog_biome)) {
            const auto& bb = std::get<SBogBiome>(config_.bog_biome);
            if (noise < bb.pool_threshold) return bb.pool_tile;
            if (noise > bb.reed_threshold) return bb.reed_tile;
            return bb.ground_tile;
        }
        return "bog_ground";
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
        if (std::holds_alternative<SHillsBiome>(config_.hills_biome)) {
            const auto& hb = std::get<SHillsBiome>(config_.hills_biome);
            const double scatter = HashNoise(x, y, noise_seed_ + 163);
            const double cover = Fbm(x * 0.10, y * 0.10, noise_seed_ + 167);
            if (scatter < hb.rock_density) {
                return hb.rock_tile;
            }
            if (scatter < hb.rock_density + hb.heather_density && cover > hb.cover_threshold) {
                return hb.heather_tile;
            }
            if (scatter < hb.rock_density + hb.heather_density + hb.scrub_density * (1.0 + cover)) {
                return hb.scrub_tile;
            }
            return hb.base_tile;
        }
        return "slope";
    }

    case Ter::Mountain: {
        // The smoothed height, its outline worn by the noise; past the
        // rock line the mountain is impassable
        const double height = HeightAt(x, y) + 0.4 * (noise - 0.5);

        if (height > 0.45) {
            return "mountain_rock";
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

// Connect entrance approaches to a shared path network. Run after terrain
// repair so every reachable town pocket can be joined without cutting walls.
void CWilderness::ConnectTownDoors(CMap& level) const
{
    std::vector<bool> buildings(width_ * height_);
    for (const auto& region : regions_) {
        for (const auto& b : region.buildings) {
            for (auto y = b.corner1.y; y <= b.corner2.y; ++y) {
                for (auto x = b.corner1.x; x <= b.corner2.x; ++x) { buildings[Index(x, y)] = true; }
            }
        }
    }
    for (std::size_t id = 0; id < regions_.size(); ++id) {
        const auto& houses = regions_[id].buildings;
        if (houses.empty()) { continue; }
        std::vector<bool> network(width_ * height_);
        for (const auto& b : houses) {
            const auto door = b.door;
            const Position approach = door.x == b.corner1.x ? Position(door.x - 1, door.y) :
                                      door.x == b.corner2.x ? Position(door.x + 1, door.y) :
                                      door.y == b.corner1.y ? Position(door.x, door.y - 1) : Position(door.x, door.y + 1);
            const int start = Index(approach.x, approach.y);
            if (&b == &houses.front()) {
                network[start] = true;
                level.SetCell(approach, "corridor");
                continue;
            }
            std::vector<int> parent;
            int end = -1;
            // Prefer cardinal paths inside the town. A fallback may follow
            // repaired walkable ground outside it to join fragmented regions.
            for (int pass = 0; pass < 2 && end < 0; ++pass) {
                parent.assign(width_ * height_, -1);
                parent[start] = start;
                std::vector<int> queue{start};
                for (std::size_t index = 0; index < queue.size() && end < 0; ++index) {
                    const int cell = queue[index];
                    if (network[cell]) { end = cell; break; }
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if ((dx == 0 && dy == 0) || (pass == 0 && dx != 0 && dy != 0)) { continue; }
                            const int x = cell % width_ + dx, y = cell / width_ + dy;
                            if (x < 0 || y < 0 || x >= width_ || y >= height_) { continue; }
                            const int next = Index(x, y);
                            if (parent[next] >= 0 || buildings[next] || !level.get(x, y).isPassable() ||
                                    (pass == 0 && region_id_[next] != static_cast<int>(id))) { continue; }
                            parent[next] = cell;
                            queue.push_back(next);
                        }
                    }
                }
            }
            if (end < 0) { continue; }
            for (int cell = end;; cell = parent[cell]) {
                network[cell] = true;
                const auto type = level.get(cell % width_, cell / width_).getType();
                const bool cultivated = regions_[region_id_[cell]].terrain == Ter::Field;
                if (!cultivated && type != "bridge" && type != "door_open" && type != "corridor") {
                    level.SetCell(cell % width_, cell / width_, "corridor");
                }
                if (cell == start) { break; }
            }
        }
    }
}

// Generate gently winding field lanes, joining the first existing village road
// they reach. Obstacles use safe waypoints rather than being overwritten.
void CWilderness::ConnectFieldRoads(CMap& level) const
{
    if (!with_buildings_) return;
    std::vector<bool> protected_cell(width_ * height_), network(width_ * height_);
    std::vector<Position> roads;
    int anchor = -1;
    for (const auto& region : regions_) {
        for (const auto& b : region.buildings) {
            for (auto y = b.corner1.y; y <= b.corner2.y; ++y) {
                for (auto x = b.corner1.x; x <= b.corner2.x; ++x) { protected_cell[Index(x, y)] = true; }
            }
        }
    }
    for (int cell = 0; cell < width_ * height_; ++cell) {
        if (regions_[region_id_[cell]].terrain != Ter::Town || protected_cell[cell]) { continue; }
        const auto& tile = level.get(cell % width_, cell / width_);
        if (tile.getType() == "corridor") {
            network[cell] = true;
            roads.emplace_back(cell % width_, cell / width_);
        }
        if (tile.isPassable() && (anchor < 0 || edge_dist_[cell] > edge_dist_[anchor])) { anchor = cell; }
    }
    if (roads.empty() && anchor >= 0) {
        level.SetCell(anchor % width_, anchor / width_, "corridor");
        network[anchor] = true;
        roads.emplace_back(anchor % width_, anchor / width_);
    }
    if (roads.empty()) { return; }
    auto cost = [&](Position p) {
        if (!level.inside(p) || protected_cell[Index(p.x, p.y)]) { return 0; }
        const auto& tile = level.get(p);
        if (tile.getType() == "tree") { return 4; }
        if (tile.getType() == "water") { return 0; }
        return tile.isPassable() ? 1 : 0;
    };
    auto append_winding = [&](std::vector<Position>& path, Position start, Position end) {
        std::vector<Position> points;
        BuildWindingPath(points, level, start, end, 8);
        for (auto point : points) {
            if (path.empty() || path.back() != point) { path.push_back(point); }
        }
    };
    for (std::size_t id = 0; id < regions_.size(); ++id) {
        const auto& field = regions_[id];
        if (field.terrain != Ter::Field || field.gate < 0) { continue; }
        const Position gate(field.gate % width_, field.gate / width_);
        const auto target = *std::ranges::min_element(roads, [&](Position a, Position b) {
            return gate.Distance(a) < gate.Distance(b);
        });
        Position approach;
        auto best = std::numeric_limits<std::size_t>::max();
        for (const auto [dx, dy] : std::array<std::pair<int, int>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
            const Position next(gate.x + dx, gate.y + dy);
            if (cost(next) > 0 && region_id_[Index(next.x, next.y)] != static_cast<int>(id) && next.Distance(target) < best) {
                best = next.Distance(target);
                approach = next;
            }
        }
        if (approach.x == Position::invalid) { continue; }
        std::vector<Position> path;
        for (int attempt = 0; attempt < 6; ++attempt) {
            path.clear();
            append_winding(path, approach, target);
            const auto joined = std::ranges::find_if(path, [&](Position p) { return network[Index(p.x, p.y)]; });
            if (joined != path.end()) { path.erase(joined + 1, path.end()); }
            if (std::ranges::all_of(path, [&](Position p) { return cost(p) > 0; })) { break; }
            path.clear();
        }
        if (path.empty()) {
            // Route around hedges, buildings, mountains and ravines. Each
            // simplify the safe route before winding each longer segment.
            std::vector<int> distance(width_ * height_, std::numeric_limits<int>::max());
            std::vector<int> parent(width_ * height_, -1);
            using Entry = std::pair<int, int>;
            std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
            const int start = Index(approach.x, approach.y);
            distance[start] = 0;
            pending.emplace(0, start);
            int end = -1;
            while (!pending.empty()) {
                const auto [value, cell] = pending.top(); pending.pop();
                if (value != distance[cell]) { continue; }
                if (network[cell]) { end = cell; break; }
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) { continue; }
                        const Position next(cell % width_ + dx, cell / width_ + dy);
                        const int step = cost(next);
                        if (step == 0) { continue; }
                        const int index = Index(next.x, next.y);
                        if (value + step < distance[index]) {
                            distance[index] = value + step;
                            parent[index] = cell;
                            pending.emplace(value + step, index);
                        }
                    }
                }
            }
            if (end < 0) { continue; }
            std::vector<Position> waypoints;
            for (int cell = end; cell >= 0; cell = parent[cell]) { waypoints.emplace_back(cell % width_, cell / width_); }
            std::reverse(waypoints.begin(), waypoints.end());
            for (std::size_t i = 0; i + 1 < waypoints.size();) {
                // Remove point-by-point search turns using clear sightlines.
                auto next = std::min(i + 24, waypoints.size() - 1);
                std::vector<Position> straight;
                for (; next > i; --next) {
                    straight = waypoints[i].BuildBresenhamLine(waypoints[next]);
                    if (std::ranges::all_of(straight, [&](Position p) { return cost(p) > 0; })) { break; }
                }
                std::vector<Position> segment;
                append_winding(segment, waypoints[i], waypoints[next]);
                if (!std::ranges::all_of(segment, [&](Position p) { return cost(p) > 0; })) {
                    segment = straight; // Tight obstacles leave no room to bend.
                }
                for (auto p : segment) {
                    if (path.empty() || path.back() != p) { path.push_back(p); }
                }
                i = next;
            }
            if (waypoints.size() == 1) { path = waypoints; }
        }
        for (auto p : path) {
            const int cell = Index(p.x, p.y);
            const auto type = level.get(p).getType();
            const bool cultivated = regions_[region_id_[cell]].terrain == Ter::Field;
            if (!cultivated && type != "door_open" && type != "corridor" && type != "bridge") {
                level.SetCell(p, type == "water" ? "bridge" : "corridor");
            }
            if (!network[cell]) { roads.push_back(p); network[cell] = true; }
        }
    }
}

// Collect after connectivity repairs and farm roads, since opening a hedge
// creates additional cultivable cells. Preserve the original region boundaries.
std::vector<std::vector<Position>> CWilderness::Fields(const CMap& level) const
{
    std::vector<std::vector<Position>> fields(regions_.size());
    for (std::size_t cell = 0; cell < region_id_.size(); ++cell) {
        const Position pos{cell % width_, cell / width_};
        if (level.get(pos).getType() == "field") fields[region_id_[cell]].push_back(pos);
    }
    std::erase_if(fields, [](const auto& cells) { return cells.empty(); });
    return fields;
}

// Collected last, as Fields() is: the roads and the openings made through
// the map take cells of the grass regions.
std::vector<std::vector<Position>> CWilderness::Meadows(const CMap& level) const
{
    std::vector<std::vector<Position>> meadows(regions_.size());
    for (std::size_t cell = 0; cell < region_id_.size(); ++cell) {
        const Position pos{cell % width_, cell / width_};
        const auto type = level.get(pos).getType();
        if (regions_[region_id_[cell]].terrain == Ter::Grass && level.get(pos).isPassable() &&
                type != "corridor" && type != "bridge") {
            meadows[region_id_[cell]].push_back(pos);
        }
    }
    std::erase_if(meadows, [](const auto& cells) { return cells.empty(); });
    return meadows;
}

SWildernessInfo CWilderness::Apply(CMap & level) const
{
    SWildernessInfo info;
    info.regions = regions_.size();
    info.adjacency = adjacency_set_.size();
    info.difficulty = CMatrix<int>(level.getSize(), 0);

    CMatrix<bool> forest_mask(width_, height_, false);
    CMatrix<bool> bog_mask(width_, height_, false);
    CMatrix<bool> hills_mask(width_, height_, false);
    CMatrix<bool> grass_mask(width_, height_, false);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const auto terrain = regions_[region_id_[Index(x, y)]].terrain;
            if (terrain == Ter::Forest) {
                forest_mask.set(x, y, true);
            } else if (terrain == Ter::Bog) {
                bog_mask.set(x, y, true);
            } else if (terrain == Ter::Hills) {
                hills_mask.set(x, y, true);
            } else if (terrain == Ter::Grass) {
                grass_mask.set(x, y, true);
            } else {
                const auto type = TileAt(x, y);
                if (type == "slope") {
                    const double elevation = HeightAt(x, y);
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
                } else if (type == "chasm" || type == "chasm_rim" || type == "chasm_wall") {
                    auto data = CTileData::get(type).second;
                    const int depth = chasm_depth_[Index(x, y)];
                    const double shade = std::clamp((depth - 1) / 3.0, 0.0, 1.0);
                    std::uint32_t colour = 0;
                    constexpr std::uint32_t rim = 0x8E7862, deep = 0x211E28;
                    for (int shift : {16, 8, 0}) {
                        const double channel = ((rim >> shift) & 0xFF) * (1.0 - shade) + ((deep >> shift) & 0xFF) * shade;
                        colour |= static_cast<std::uint32_t>(std::lround(channel)) << shift;
                    }
                    data.rgb_color = GetJitteredColor(colour);
                    if (type == "chasm_rim" && HashNoise(x, y, noise_seed_ + 197) < 0.25) {
                        data.glyph = U'^';
                    }
                    level.get(x, y) = CTile(type, data);
                } else {
                    level.SetCell(static_cast<std::size_t>(x), static_cast<std::size_t>(y), type);
                }
            }
            info.difficulty.set(static_cast<std::size_t>(x), static_cast<std::size_t>(y),
                                regions_[region_id_[Index(x, y)]].difficulty);
        }
    }

    CMatrix<double> elev_mat(level.getSize(), 0.0);
    CMatrix<int> reg_mat(level.getSize(), 0);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            elev_mat.set(x, y, elevation_[Index(x, y)]);
            reg_mat.set(x, y, region_id_[Index(x, y)]);
        }
    }

    FillBiome(level, forest_mask, config_.forest_biome, &elev_mat, &reg_mat, noise_seed_);
    FillBiome(level, bog_mask, config_.bog_biome, &elev_mat, &reg_mat, noise_seed_);
    FillBiome(level, grass_mask, config_.grass_biome, &elev_mat, &reg_mat, noise_seed_);

    if (std::holds_alternative<SHillsBiome>(config_.hills_biome)) {
        auto hills = std::get<SHillsBiome>(config_.hills_biome);
        const bool want_features = hills.animal_trails || hills.fallen_logs;
        hills.animal_trails = false;
        hills.fallen_logs = false;
        FillHills(level, hills_mask, hills, &elev_mat, &reg_mat, noise_seed_);
        if (want_features) {
            AddHillFeatures(level);
        }
    } else {
        FillBiome(level, hills_mask, config_.hills_biome, &elev_mat, &reg_mat, noise_seed_);
    }

    for (std::size_t i = 0; i < regions_.size(); ++i) {
        const Region & region = regions_[i];

        if (region.terrain == Ter::Field && region.gate >= 0) {
            info.field_gates.emplace_back(region.gate % width_, region.gate / width_);
        }
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
// to get through a forest. Lakes, mountains, peaks and chasms are never
// opened; paths must follow their shores or other walkable ground. See MakeReachable() in rl.connectivity.
inline constexpr std::array<SOpening, 2> MultiRegionOpenings = {{
        {"hedge", 3, "field"},
        {"tree", 4, "grass"}
    }
};

inline constexpr auto& WildernessOpenings = MultiRegionOpenings;

// Generate a wilderness over the whole map: seed points scattered at
// random, every cell in the region of the point nearest to it, the
// difficulties by flood fill over the Delaunay adjacency of the regions,
// and the terrain of each region picked from its difficulty. The town sits
// at difficulty 0, the hardest peaks at 9, and lakes and chasms go in only
// where the passable regions stay connected.
//
// The shape of the algorithm is Andrew Doull's "Wilderness generation
// using Voronoi diagrams", written for Unangband and published at
// roguelikedeveloper.blogspot.com in July 2007.
//
// The tile types field, hedge, slope, mountain_rock, peak and chasm are registered the
// first time this runs; a game that registered a type of one of those
// names itself keeps its own.
//
// @param level   The map to fill. An empty one is left alone.
// @param config  Wilderness configuration options (region count, biomes, etc.).
//
// @return The regions and their adjacency edges, the locations the
// generator laid out - the starting town, as an SLocation of type
// Town - the buildings, and the difficulty of every cell.
SMultiRegionInfo CreateMultiRegion(CMap & level, const SMultiRegionConfig& config)
{
    const int width = static_cast<int>(level.getWidth());
    const int height = static_cast<int>(level.getHeight());

    if (width == 0 || height == 0) {
        return SMultiRegionInfo();
    }

    voronoiwilderness_detail::RegisterWildernessTiles();

    int num_regions = config.num_regions;
    if (num_regions <= 0) {
        num_regions = std::max(width * height / 200, 10);
    } else {
        num_regions = std::clamp(num_regions, 4, 800);
    }

    SMultiRegionConfig effective_config = config;
    effective_config.num_regions = num_regions;

    const voronoiwilderness_detail::CWilderness wilderness(width, height, effective_config);
    SMultiRegionInfo info = wilderness.Apply(level);

    // Whatever the connectivity of the regions guaranteed, a gate may
    // still lead into a hedge and a forest may still be too dense to
    // cross. Open up whatever pockets are left, the cheapest way first.
    MakeReachable(level, MultiRegionOpenings);
    wilderness.ConnectTownDoors(level);
    wilderness.ConnectFieldRoads(level);
    info.fields = wilderness.Fields(level);
    info.meadows = wilderness.Meadows(level);

    return info;
}

// Generates a wilderness map divided into multiple Voronoi regions using default biomes.
//
// The shape of the algorithm is Andrew Doull's "Wilderness generation
// using Voronoi diagrams", written for Unangband and published at
// roguelikedeveloper.blogspot.com in July 2007.
//
// @param level           The map to fill. An empty one is left alone.
// @param num_regions     How many Voronoi regions to scatter, at least 4.
// At 0 one region is asked for per 200 cells of the map, with at least 10.
// Larger maps add regions instead of enlarging them.
// @param with_buildings  Whether to fill the town with small buildings.
// When false, agricultural field regions become uncultivated bogs.
// Each side, including walls, is 4–7 tiles long; larger towns get more
// buildings. Narrow plots that cannot hold a 4x4 building stay open.
//
// @return The regions and their adjacency edges, the locations the
// generator laid out - the starting town, as an SLocation of type
// Town - the buildings, and the difficulty of every cell.
inline SMultiRegionInfo CreateMultiRegion(CMap & level, int num_regions = 0, bool with_buildings = false)
{
    return CreateMultiRegion(level, SMultiRegionConfig{
        .num_regions = num_regions,
        .with_buildings = with_buildings
    });
}

// Backward-compatibility aliases for CreateVoronoiWilderness
inline SMultiRegionInfo CreateVoronoiWilderness(CMap & level, const SMultiRegionConfig& config)
{
    return CreateMultiRegion(level, config);
}

inline SMultiRegionInfo CreateVoronoiWilderness(CMap & level, int num_regions = 0, bool with_buildings = false)
{
    return CreateMultiRegion(level, num_regions, with_buildings);
}

} // export namespace RL

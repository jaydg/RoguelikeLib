module;

//////////////////////////////////////////////////////////////////////////
// Biome / Terrain Surface Pattern Generators
//
// Decoupled surface pattern generators that fill a masked area of a CMap
// with textured terrain (forests, bogs, rolling hills, grassland, or solid rock).
//////////////////////////////////////////////////////////////////////////

export module rl.mapgenerators.biomes;

export import rl.map;
export import rl.matrix;
export import rl.position;
import rl.tile;
import rl.randomness;
import std;

namespace RL
{

// =============================================================================
// Deterministic Hash Noise & Fractal Brownian Motion
// =============================================================================

/**
 * A value in [0, 1) for a position and a seed, the same each time, as if
 * drawn at random: the noise the biomes are filled with.
 *
 * Position and seed are combined, then mixed with MurmurHash3's 64-bit
 * finalizer, so that every bit of them reaches every bit of the value.
 * Without the finalizer, seeds a few apart - as callers derive one value
 * from `seed + 137` and another from `seed + 149` for the same tile - gave
 * values bound to each other: a sparse forest, whose few tiles of
 * vegetation fell in a narrow band of the first, never drew a tree from
 * the second.
 *
 * @param x    the position's column
 * @param y    the position's row
 * @param seed the seed of the noise
 * @return     a value in [0, 1)
 */
export double HashNoise(const int x, const int y, const std::uint64_t seed)
{
    std::uint64_t hash = seed;
    hash ^= static_cast<std::uint64_t>(x) * 0x9E3779B97F4A7C15ULL;
    hash = std::rotl(hash, 31);
    hash ^= static_cast<std::uint64_t>(y) * 0xC2B2AE3D27D4EB4FULL;

    hash ^= hash >> 33;
    hash *= 0xFF51AFD7ED558CCDULL;
    hash ^= hash >> 33;
    hash *= 0xC4CEB9FE1A85EC53ULL;
    hash ^= hash >> 33;

    return static_cast<double>(hash >> 11) * (1.0 / 9007199254740992.0);
}

export double SmoothNoise(const double x, const double y, const std::uint64_t seed)
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

export double Fbm(double x, double y, const std::uint64_t seed, const int octaves = 4)
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

// =============================================================================
// Biome Tile Registration
// =============================================================================

export void RegisterBiomeTiles()
{
    const auto add = [](const std::string_view name, const STileData & data) {
        if (!CTileData::has(name)) {
            CTileData::RegisterTile(name, data);
        }
    };

    add("field", {U',', 0xD9C463, true, true});                  // farmland
    add("bog_ground", {U'.', 0x6C7156, true, true});
    add("bog_reeds", {U'&', 0x6C7C59, false, true});
    add("bog_pool", {U'~', 0x356B70, true, true});
    add("hedge", {U'#', 0x2E7D32, false, false});                // field boundary
    add("slope", {CTileData::get("grass").second.glyph, 0xA7CC7C, true, true}); // elevation-tinted grass
    add("scrub", {U'&', 0x84945A, false, true});                 // hillside bushes
    add("heather", {U'*', 0xB889B4, true, true});                // sparse flowering ground cover
    add("trail", {U'.', 0x929B70, true, true});                  // faint worn trail
    add("animal_trail", {U'.', 0x929B70, true, true});           // faint worn trail
    add("fallen_log", {U'=', 0x8B6B43, true, true});             // passable horizontal log
    add("fallen_log_vertical", {U'|', 0x8B6B43, true, true});    // passable vertical log
    add("mountain_rock", {U'^', 0x888888, false, false});        // impassable mountain rock
    add("peak", {U'*', 0xF5F5F5, false, false});                 // snow summit
    add("chasm", {U'·', 0x28242E, true, false});                 // chasm depth
    add("chasm_rim", {U'#', 0x8E7862, true, false});             // broken cliff edge
    add("chasm_wall", {U':', 0x625047, true, false});            // descending face
}

// =============================================================================
// Biome Parameter Structs
// =============================================================================

export struct SSolidBiome {
    std::string_view tile = "wall";
};

export struct SForestBiome {
    float tree_density = 0.70f;       ///< Share of vegetation that is trees vs. plants (0.0-1.0)
    float rock_density = 0.05f;       ///< Probability of scattered rocks (0.0-1.0)
    float vegetation_density = 0.60f; ///< Target vegetation coverage (0.0-1.0)
    float patchiness = 0.30f;         ///< Low-frequency FBM thicket modulation (0.0 = uniform)
    std::string_view base_tile = "grass";
    std::string_view tree_tile = "tree";
    std::string_view plant_tile = "plant";
    std::string_view rock_tile = "rock";
};

export struct SBogBiome {
    float pool_threshold = 0.35f;     ///< Noise threshold under which bog pools form
    float reed_threshold = 0.60f;     ///< Noise threshold above which bog reeds form
    std::string_view ground_tile = "bog_ground";
    std::string_view pool_tile = "bog_pool";
    std::string_view reed_tile = "bog_reeds";
};

export struct SHillsBiome {
    float rock_density = 0.02f;
    float heather_density = 0.04f;
    float scrub_density = 0.10f;
    float cover_threshold = 0.55f;
    bool animal_trails = true;
    bool fallen_logs = true;
    std::string_view base_tile = "slope";
    std::string_view rock_tile = "rock";
    std::string_view heather_tile = "heather";
    std::string_view scrub_tile = "scrub";
};

export struct SGrasslandBiome {
    float tree_threshold = 0.88f;     ///< Coherent noise threshold for sparse trees
    float plant_threshold = 0.06f;    ///< Coherent noise threshold for sparse plants
    std::string_view base_tile = "grass";
    std::string_view tree_tile = "tree";
    std::string_view plant_tile = "plant";
};

export using CustomBiomeFn = std::function<void(CMap&, const CMatrix<bool>&)>;

export using TerrainBiome = std::variant<
    SSolidBiome,
    SForestBiome,
    SBogBiome,
    SHillsBiome,
    SGrasslandBiome,
    CustomBiomeFn
>;

// =============================================================================
// Masked Fill Functions
// =============================================================================

export void FillSolid(CMap& map, const CMatrix<bool>& mask, const SSolidBiome& params = {})
{
    const std::size_t width = std::min(map.getWidth(), mask.getWidth());
    const std::size_t height = std::min(map.getHeight(), mask.getHeight());

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (mask.get(x, y)) {
                map.SetCell(x, y, params.tile);
            }
        }
    }
}

export void FillForest(
    CMap& map,
    const CMatrix<bool>& mask,
    const SForestBiome& params = {},
    std::uint64_t seed = 0
)
{
    RegisterBiomeTiles();
    const std::size_t width = std::min(map.getWidth(), mask.getWidth());
    const std::size_t height = std::min(map.getHeight(), mask.getHeight());

    if (seed == 0) {
        seed = static_cast<std::uint64_t>(Random(0x7fffffff));
    }

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (!mask.get(x, y)) {
                continue;
            }

            const int ix = static_cast<int>(x);
            const int iy = static_cast<int>(y);

            double vegetation = params.vegetation_density;
            if (params.patchiness > 0.0f) {
                const double fbm = Fbm(ix * 0.07, iy * 0.07, seed + 131);
                vegetation = std::clamp(
                    static_cast<double>(params.vegetation_density) +
                    static_cast<double>(params.patchiness) * (fbm - 0.5),
                    0.0, 1.0);
            }

            const double scatter = HashNoise(ix, iy, seed + 137);
            if (scatter < params.rock_density) {
                map.SetCell(x, y, params.rock_tile);
            } else if (scatter < params.rock_density + (1.0 - params.rock_density) * vegetation) {
                const bool is_tree = HashNoise(ix, iy, seed + 149) < params.tree_density;
                map.SetCell(x, y, is_tree ? params.tree_tile : params.plant_tile);
            } else {
                map.SetCell(x, y, params.base_tile);
            }
        }
    }
}

export void FillBog(
    CMap& map,
    const CMatrix<bool>& mask,
    const SBogBiome& params = {},
    std::uint64_t seed = 0
)
{
    RegisterBiomeTiles();
    const std::size_t width = std::min(map.getWidth(), mask.getWidth());
    const std::size_t height = std::min(map.getHeight(), mask.getHeight());

    if (seed == 0) {
        seed = static_cast<std::uint64_t>(Random(0x7fffffff));
    }

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (!mask.get(x, y)) {
                continue;
            }

            const double noise = Fbm(static_cast<int>(x) * 0.25, static_cast<int>(y) * 0.25, seed);
            if (noise < params.pool_threshold) {
                map.SetCell(x, y, params.pool_tile);
            } else if (noise > params.reed_threshold) {
                map.SetCell(x, y, params.reed_tile);
            } else {
                map.SetCell(x, y, params.ground_tile);
            }
        }
    }
}

export void FillGrassland(
    CMap& map,
    const CMatrix<bool>& mask,
    const SGrasslandBiome& params = {},
    std::uint64_t seed = 0
)
{
    RegisterBiomeTiles();
    const std::size_t width = std::min(map.getWidth(), mask.getWidth());
    const std::size_t height = std::min(map.getHeight(), mask.getHeight());

    if (seed == 0) {
        seed = static_cast<std::uint64_t>(Random(0x7fffffff));
    }

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (!mask.get(x, y)) {
                continue;
            }

            const double noise = Fbm(static_cast<int>(x) * 0.25, static_cast<int>(y) * 0.25, seed);
            if (noise > params.tree_threshold) {
                map.SetCell(x, y, params.tree_tile);
            } else if (noise < params.plant_threshold) {
                map.SetCell(x, y, params.plant_tile);
            } else {
                map.SetCell(x, y, params.base_tile);
            }
        }
    }
}

export void FillHills(
    CMap& map,
    const CMatrix<bool>& mask,
    const SHillsBiome& params = {},
    const CMatrix<double>* elevation = nullptr,
    const CMatrix<int>* region_id = nullptr,
    std::uint64_t seed = 0
)
{
    RegisterBiomeTiles();
    const std::size_t width = std::min(map.getWidth(), mask.getWidth());
    const std::size_t height = std::min(map.getHeight(), mask.getHeight());

    if (seed == 0) {
        seed = static_cast<std::uint64_t>(Random(0x7fffffff));
    }

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (!mask.get(x, y)) {
                continue;
            }

            const int ix = static_cast<int>(x);
            const int iy = static_cast<int>(y);
            const double scatter = HashNoise(ix, iy, seed + 163);
            const double cover = Fbm(ix * 0.10, iy * 0.10, seed + 167);

            if (scatter < params.rock_density) {
                map.SetCell(x, y, params.rock_tile);
            } else if (scatter < params.rock_density + params.heather_density && cover > params.cover_threshold) {
                map.SetCell(x, y, params.heather_tile);
            } else if (scatter < params.rock_density + params.heather_density + params.scrub_density * (1.0 + cover)) {
                map.SetCell(x, y, params.scrub_tile);
            } else {
                // Base slope with elevation tinting if elevation data is present
                if (elevation && CTileData::has(params.base_tile)) {
                    const double elev = 0.5 * elevation->get(x, y) + 0.5 * Fbm(ix * 0.15, iy * 0.15, seed + 99);
                    const double blend = std::clamp(elev, 0.0, 1.0);
                    const auto low = CTileData::get("grass").second.rgb_color;
                    constexpr std::uint32_t high = 0xA08A5B;
                    std::uint32_t colour = 0;
                    for (int shift : {16, 8, 0}) {
                        const double channel = ((low >> shift) & 0xFF) * (1.0 - blend) +
                                               ((high >> shift) & 0xFF) * blend;
                        colour |= static_cast<std::uint32_t>(std::lround(channel)) << shift;
                    }
                    auto data = CTileData::get(params.base_tile).second;
                    data.rgb_color = GetJitteredColor(colour);
                    map.get(x, y) = CTile(params.base_tile, data);
                } else {
                    map.SetCell(x, y, params.base_tile);
                }
            }
        }
    }

    // Optional animal trails and fallen logs
    if (params.animal_trails || params.fallen_logs) {
        auto ground = [&](std::size_t x, std::size_t y) {
            const auto type = map.get(x, y).getType();
            return type == params.base_tile || type == params.scrub_tile || type == params.heather_tile;
        };

        std::vector<Position> masked_cells;
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                if (mask.get(x, y) && ground(x, y)) {
                    masked_cells.emplace_back(x, y);
                }
            }
        }

        if (params.animal_trails && masked_cells.size() >= 80) {
            const auto start_idx = static_cast<std::size_t>(HashNoise(0, 0, seed) * masked_cells.size());
            const Position start = masked_cells[start_idx % masked_cells.size()];
            if (HashNoise(static_cast<int>(start.x), 5, seed) < 0.30) {
                const int length = std::clamp(static_cast<int>(0.75 * std::sqrt(masked_cells.size())), 6, 16);
                constexpr std::array<std::pair<int, int>, 4> directions{{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
                int heading = static_cast<int>(HashNoise(static_cast<int>(start.x), 0, seed) * 4);
                Position current = start;
                std::vector<Position> trail{start};
                CMatrix<bool> visited(map.getSize(), false);
                visited.set(start.x, start.y, true);

                for (int step = 1; step < length; ++step) {
                    int best_dir = heading;
                    Position best_pos;
                    double lowest = std::numeric_limits<double>::max();
                    for (int d = 0; d < 4; ++d) {
                        if (d == (heading + 2) % 4) continue;
                        const int nx = static_cast<int>(current.x) + directions[d].first;
                        const int ny = static_cast<int>(current.y) + directions[d].second;
                        if (!map.inside(nx, ny) || !mask.get(nx, ny) || visited.get(nx, ny) || !ground(nx, ny)) {
                            continue;
                        }
                        if (region_id && region_id->get(nx, ny) != region_id->get(start.x, start.y)) {
                            continue;
                        }
                        const double turn = step % 4 == 0 ? 0.20 : 0.80;
                        double score = 0.45 * HashNoise(nx, ny, seed + step) + (d == heading ? 0 : turn);
                        if (elevation) {
                            score += 2.0 * std::abs(elevation->get(nx, ny) - elevation->get(current.x, current.y));
                        }
                        if (score < lowest) {
                            lowest = score;
                            best_pos = Position(nx, ny);
                            best_dir = d;
                        }
                    }
                    if (best_pos.x == Position::invalid) break;
                    current = best_pos;
                    heading = best_dir;
                    visited.set(current.x, current.y, true);
                    trail.push_back(current);
                }

                if (trail.size() >= 6) {
                    for (const auto& p : trail) {
                        map.SetCell(p, "animal_trail");
                    }
                }
            }
        }

        if (params.fallen_logs && masked_cells.size() >= 40) {
            int remaining = std::max(1, static_cast<int>(masked_cells.size() / 300));
            for (const auto& cell : masked_cells) {
                if (remaining == 0) break;
                const int cx = static_cast<int>(cell.x);
                const int cy = static_cast<int>(cell.y);
                if (HashNoise(cx, 2, seed) > 0.03 || !ground(cx, cy)) continue;

                const bool vertical = HashNoise(cx, 3, seed) < 0.5;
                const int log_size = HashNoise(cx, 4, seed) < 0.5 ? 2 : 3;
                std::vector<Position> log_cells;
                for (int offset = 0; offset < log_size; ++offset) {
                    const int lx = cx + (vertical ? 0 : offset);
                    const int ly = cy + (vertical ? offset : 0);
                    if (!map.inside(lx, ly) || !mask.get(lx, ly) || !ground(lx, ly)) break;
                    if (region_id && region_id->get(lx, ly) != region_id->get(cx, cy)) break;
                    log_cells.emplace_back(lx, ly);
                }
                if (log_cells.size() == static_cast<std::size_t>(log_size)) {
                    for (const auto& lp : log_cells) {
                        map.SetCell(lp, vertical ? "fallen_log_vertical" : "fallen_log");
                    }
                    --remaining;
                }
            }
        }
    }
}

export void FillBiome(
    CMap& map,
    const CMatrix<bool>& mask,
    const TerrainBiome& biome,
    const CMatrix<double>* elevation = nullptr,
    const CMatrix<int>* region_id = nullptr,
    std::uint64_t seed = 0
)
{
    std::visit([&](const auto& b) {
        using T = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<T, SSolidBiome>) {
            FillSolid(map, mask, b);
        } else if constexpr (std::is_same_v<T, SForestBiome>) {
            FillForest(map, mask, b, seed);
        } else if constexpr (std::is_same_v<T, SBogBiome>) {
            FillBog(map, mask, b, seed);
        } else if constexpr (std::is_same_v<T, SGrasslandBiome>) {
            FillGrassland(map, mask, b, seed);
        } else if constexpr (std::is_same_v<T, SHillsBiome>) {
            FillHills(map, mask, b, elevation, region_id, seed);
        } else if constexpr (std::is_same_v<T, CustomBiomeFn>) {
            b(map, mask);
        }
    }, biome);
}

} // namespace RL

import rl.mapgenerators.forest;
import rl.mapgenerators.caveandglade;
import rl.mapgenerators.biomes;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

void TestForestRoads()
{
    std::size_t centres = 0, paved_centres = 0, base_roads = 0, extra_roads = 0;
    for (unsigned seed = 0; seed < 24; ++seed) {
        const RL::Size size(seed % 2 ? 120 : 79, seed % 2 ? 80 : 50);
        RL::CMap map(size), base(size), replay(size);
        RL::InitRandomness(seed);
        const auto locations = RL::GenerateForest(map, 0.7f, 0.05f, 5, 8, 1, 3);
        RL::InitRandomness(seed);
        RL::GenerateForest(base, 0.7f, 0.05f, 5, 0, 1, 3);
        RL::InitRandomness(seed);
        RL::GenerateForest(replay, 0.7f, 0.05f, 5, 8, 1, 3);
        std::vector<bool> seen(size.x * size.y);
        std::vector<RL::Position> queue{locations.front().center};
        seen[queue.front().y * size.x + queue.front().x] = true;
        for (std::size_t i = 0; i < queue.size(); ++i) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const RL::Position next(queue[i].x + dx, queue[i].y + dy);
                    if (!map.inside(next) || !map.get(next).isPassable()) continue;
                    const auto cell = next.y * size.x + next.x;
                    if (!seen[cell]) { seen[cell] = true; queue.push_back(next); }
                }
            }
        }
        bool connected = true, reproducible = true;
        for (const auto& clearing : locations) {
            connected &= seen[clearing.center.y * size.x + clearing.center.x] && map.get(clearing.center).isPassable();
            ++centres;
            paved_centres += map.get(clearing.center).getType() == "corridor";
        }
        for (std::size_t y = 0; y < size.y; ++y) {
            for (std::size_t x = 0; x < size.x; ++x) {
                reproducible &= map.get(x, y).getType() == replay.get(x, y).getType();
                base_roads += base.get(x, y).getType() == "corridor" || base.get(x, y).getType() == "bridge";
                extra_roads += map.get(x, y).getType() == "corridor" || map.get(x, y).getType() == "bridge";
            }
        }
        Check(connected, "clearing entrances stay reachable, including rivers and scattered rocks");
        Check(reproducible, "forest road networks reproduce from their seed");
    }
    Check(paved_centres * 10 < centres, "forest roads leave clearing centres predominantly unpaved");
    Check(extra_roads < base_roads * 2, "additional routes share substantial portions of the existing road network");
}

void TestBiomes()
{
    // 1. Masked Solid fill
    {
        RL::CMap map(RL::Size(20, 20));
        map.Clear("grass");
        RL::CMatrix<bool> mask(map.getSize(), false);
        for (std::size_t y = 5; y < 15; ++y) {
            for (std::size_t x = 5; x < 15; ++x) {
                mask.set(x, y, true);
            }
        }
        RL::FillSolid(map, mask, RL::SSolidBiome{"wall"});
        bool solid_intact = true, outside_intact = true;
        for (std::size_t y = 0; y < 20; ++y) {
            for (std::size_t x = 0; x < 20; ++x) {
                if (x >= 5 && x < 15 && y >= 5 && y < 15) {
                    solid_intact &= map.get(x, y).getType() == "wall";
                } else {
                    outside_intact &= map.get(x, y).getType() == "grass";
                }
            }
        }
        Check(solid_intact && outside_intact, "FillSolid populates only masked area and preserves surroundings");
    }

    // 2. Masked Forest fill
    {
        RL::CMap map(RL::Size(40, 40));
        map.Clear("room");
        RL::CMatrix<bool> mask(map.getSize(), false);
        for (std::size_t y = 10; y < 30; ++y) {
            for (std::size_t x = 10; x < 30; ++x) {
                mask.set(x, y, true);
            }
        }
        RL::FillForest(map, mask, RL::SForestBiome{
            .tree_density = 0.8f,
            .rock_density = 0.1f,
            .vegetation_density = 0.7f
        }, 12345);
        bool outside_preserved = true;
        std::size_t trees = 0, plants = 0, rocks = 0, grass = 0;
        for (std::size_t y = 0; y < 40; ++y) {
            for (std::size_t x = 0; x < 40; ++x) {
                if (x >= 10 && x < 30 && y >= 10 && y < 30) {
                    const auto type = map.get(x, y).getType();
                    trees += type == "tree";
                    plants += type == "plant";
                    rocks += type == "rock";
                    grass += type == "grass";
                } else {
                    outside_preserved &= map.get(x, y).getType() == "room";
                }
            }
        }
        Check(outside_preserved, "FillForest preserves unmasked surroundings");
        Check(trees > 0 && plants > 0 && rocks > 0 && grass > 0, "FillForest produces trees, plants, rocks, and grass");
        Check(trees > plants, "FillForest respects tree_density ratio");
    }

    // 3. Masked Bog fill
    {
        RL::CMap map(RL::Size(40, 40));
        map.Clear("grass");
        RL::CMatrix<bool> mask(map.getSize(), true);
        RL::FillBog(map, mask, RL::SBogBiome{}, 54321);
        std::size_t pools = 0, reeds = 0, ground = 0;
        for (std::size_t y = 0; y < 40; ++y) {
            for (std::size_t x = 0; x < 40; ++x) {
                const auto type = map.get(x, y).getType();
                pools += type == "bog_pool";
                reeds += type == "bog_reeds";
                ground += type == "bog_ground";
            }
        }
        Check(pools > 0 && reeds > 0 && ground > 0, "FillBog generates pools, reeds, and bog ground");
    }

    // 4. FillBiome variant dispatch
    {
        RL::CMap map(RL::Size(30, 30));
        map.Clear("grass");
        RL::CMatrix<bool> mask(map.getSize(), true);
        RL::FillBiome(map, mask, RL::SBogBiome{}, nullptr, nullptr, 999);
        std::size_t bog_count = 0;
        for (std::size_t y = 0; y < 30; ++y) {
            for (std::size_t x = 0; x < 30; ++x) {
                const auto type = map.get(x, y).getType();
                bog_count += type == "bog_pool" || type == "bog_reeds" || type == "bog_ground";
            }
        }
        Check(bog_count == 30 * 30, "FillBiome variant dispatcher correctly delegates to FillBog");
    }

    // 5. CreateCaveAndGlade with TerrainBiome
    {
        RL::CMap map(RL::Size(40, 40));
        RL::CreateCaveAndGlade(map, RL::SForestBiome{.vegetation_density = 0.8f}, RL::SGrasslandBiome{}, RL::GladeShape, RL::GladeWater);
        bool has_forest_trees = false, has_grass = false;
        for (std::size_t y = 0; y < 40; ++y) {
            for (std::size_t x = 0; x < 40; ++x) {
                const auto type = map.get(x, y).getType();
                has_forest_trees |= type == "tree";
                has_grass |= type == "grass";
            }
        }
        Check(has_forest_trees && has_grass, "CreateCaveAndGlade with TerrainBiome fills walls with forest and glades with grassland");
    }

    // 6. GenerateForest with Bog biome
    {
        RL::CMap map(RL::Size(79, 50));
        RL::InitRandomness(42);
        const auto locations = RL::GenerateForest(map, RL::SBogBiome{}, 4, 6, 1, 4);
        Check(!locations.empty(), "GenerateForest with SBogBiome succeeds and creates clearings");
        bool saw_bog_pool = false, saw_water = false, saw_bridge_or_road = false;
        for (std::size_t y = 0; y < map.getHeight(); ++y) {
            for (std::size_t x = 0; x < map.getWidth(); ++x) {
                const auto type = map.get(x, y).getType();
                saw_bog_pool |= type == "bog_pool";
                saw_water |= type == "water";
                saw_bridge_or_road |= type == "bridge" || type == "corridor";
            }
        }
        Check(saw_bog_pool && saw_water && saw_bridge_or_road, "GenerateForest with SBogBiome generates rivers, bridges, and bog terrain");
    }
}


module;

export module rl.mapgenerators.simplecity;

export import rl.mapgenerators.biomes;
import rl.map;
import rl.maputils;
import rl.matrix;
import rl.position;
import rl.randomness;
import rl.tile;
import std;

namespace RL
{
namespace simplecity_detail
{

void CreateSimpleCityImpl(CMap &level, const int& a_number_of_buildings, const TerrainBiome* open_ground_biome)
{
    const int min_building_width = 5;
    const int max_building_width = 10;
    const int min_building_height = 5;
    const int max_building_height = 10;

    if (level.getWidth() == 0 || level.getHeight() == 0) {
        return;
    }

    for (;;) { // until created with proper # of buildings
        level.Clear("grass");

        SRoom main;
        main.corner1.x = 0;
        main.corner1.y = 0;
        main.corner2.x = level.getWidth();
        main.corner2.y = level.getHeight();

        AddRecursiveRooms(level, "corridor", max_building_width, max_building_height, main, false);

        int build_count = 0;

        int tries = 0;

        while (build_count != a_number_of_buildings && tries < 100) {
            int size_x = max_building_width * 2;
            int size_y = max_building_height * 2;

            while (true) {
                Position pos;

                if (FindOnMapRandomRectangleOfType(level, "grass", pos, Size(size_x + 2, size_y + 2))) {
                    pos.x++;
                    pos.y++;
                    SBuilding building;
                    building.corner1 = pos;
                    building.corner2 = Position(pos.x + size_x - 1, pos.y + size_y - 1);

                    // add a door leading out
                    if (CoinToss()) {
                        if (CoinToss()) {
                            building.door = Position(building.corner1.x + 1 + Random(size_x - 2), building.corner1.y);
                        } else {
                            building.door = Position(building.corner1.x + 1 + Random(size_x - 2), building.corner2.y);
                        }
                    } else {
                        if (CoinToss()) {
                            building.door = Position(building.corner1.x, building.corner1.y + 1 + Random(size_y - 2));
                        } else {
                            building.door = Position(building.corner2.x, building.corner1.y + 1 + Random(size_y - 2));
                        }
                    }

                    CreateBuildingInterior(level, building);

                    build_count++;

                    if (build_count == a_number_of_buildings) {
                        break;
                    }
                } else {
                    if (CoinToss()) {
                        size_x--;
                    } else {
                        size_y--;
                    }

                    if (size_x <= min_building_width || size_y <= min_building_height) {
                        tries++;
                        break;
                    }
                }
            }
        }

        if (tries < 100) {
            if (open_ground_biome) {
                CMatrix<bool> mask(level.getWidth(), level.getHeight(), false);
                for (std::size_t y = 0; y < level.getHeight(); ++y) {
                    for (std::size_t x = 0; x < level.getWidth(); ++x) {
                        if (level.get(x, y).getType() == "grass") {
                            mask.set(x, y, true);
                        }
                    }
                }
                FillBiome(level, mask, *open_ground_biome);
            } else {
                // plant some trees
                for (std::size_t index = 0; index < level.getWidth() * static_cast<std::size_t>(static_cast<float>(level.getHeight()) * 0.3); index++) {
                    std::size_t x = Random(level.getWidth());
                    std::size_t y = Random(level.getHeight());

                    if (level.get(x, y).getType() == "grass"
                            && level.CountNeighbors(Position(x, y), CTile::ByType("wall"), Neighbors::All8) == 0) {
                        level.SetCell(x, y, CoinToss() ? "plant" : "tree");
                    }
                }
            }

            UpdateDoorAndWindowGlyphs(level);
            return;
        }
    }
}

} // namespace simplecity_detail

export void CreateSimpleCity(CMap &level, const int& a_number_of_buildings, const TerrainBiome& open_ground_biome)
{
    simplecity_detail::CreateSimpleCityImpl(level, a_number_of_buildings, &open_ground_biome);
}

export void CreateSimpleCity(CMap &level, const int& a_number_of_buildings)
{
    simplecity_detail::CreateSimpleCityImpl(level, a_number_of_buildings, nullptr);
}

} // end of namespace RL

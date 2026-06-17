module;

export module rl.mapgenerators.simplecity;

import rl.map;
import rl.maputils;
import rl.matrix;
import rl.position;
import rl.randomness;
import rl.tile;
import std;

export namespace RL {

void CreateSimpleCity(CMap &level, const int& a_number_of_buildings)
{
    const int min_building_width = 5;
    const int max_building_width = 10;
    const int min_building_height = 5;
    const int max_building_height = 10;

    if (level.getWidth() == 0 || level.getHeight() == 0) {
        return;
    }

    for(;;) { // until created with proper # of buildings
        level.Clear("grass");

        SRoom main;
        main.corner1.x = 0;
        main.corner1.y = 0;
        main.corner2.x = level.getWidth();
        main.corner2.y = level.getHeight();

        AddRecursiveRooms(level, "corridor", max_building_width, max_building_height, main, false);

        int build_count = 0;

        int tries = 0;

        while(build_count != a_number_of_buildings && tries < 100) {
            int size_x = max_building_width * 2;
            int size_y = max_building_height * 2;

            while(true) {
                Position pos;

                if (FindOnMapRandomRectangleOfType(level, "grass", pos, Size(size_x + 2, size_y + 2))) {
                    SRoom building, smaller;
                    pos.x++;
                    pos.y++;
                    building.corner1 = pos;
                    building.corner2 = pos;
                    building.corner2.x += size_x;
                    building.corner2.y += size_y;
                    smaller = building;
                    smaller.corner1.x++;
                    smaller.corner1.y++;
                    smaller.corner2.x--;
                    smaller.corner2.y--;
                    DrawRectangleOnMap(level, building.corner1, building.corner2, "wall");
                    DrawRectangleOnMap(level, smaller.corner1, smaller.corner2, "room");
                    AddRecursiveRooms(level, "wall", 3, 3, smaller);

                    // add a doors leading out (improve to lead to nearest road)
                    if(CoinToss()) {
                        if(CoinToss()) {
                            level.SetCell(building.corner1.x + Random(size_x - 2) +1, building.corner1.y, "door_closed");
                        } else {
                            level.SetCell(building.corner1.x + Random(size_x - 2) +1, building.corner2.y - 1, "door_closed");
                        }
                    } else {
                        if(CoinToss()) {
                            level.SetCell(building.corner1.x, building.corner1.y + Random(size_y - 2) +1, "door_closed");
                        } else {
                            level.SetCell(building.corner2.x - 1, building.corner1.y + Random(size_y - 2) +1, "door_closed");
                        }
                    }

                    build_count++;

                    if(build_count == a_number_of_buildings) {
                        break;
                    }
                } else {
                    if(CoinToss()) {
                        size_x--;
                    } else {
                        size_y--;
                    }

                    if(size_x <= min_building_width || size_y <= min_building_height) {
                        tries++;
                        break;
                    }
                }
            }
        }

        if(tries < 100) {
            // plant some trees
            for (std::size_t index = 0; index < level.getWidth() * static_cast<std::size_t>(static_cast<float>(level.getHeight()) * 0.3); index++) {
                 std::size_t x = Random(level.getWidth());
                 std::size_t y = Random(level.getHeight());

                if (level.get(x, y).getType() == "grass"
                    && level.CountNeighbors(Position(x, y), CTile::ByType("wall"), Neighbors::All8) == 0)
                {
                    level.SetCell(x, y, CoinToss() ? "plant" : "tree");
                }
            }

            return;
        }
    }
}

} // end of namespace RL

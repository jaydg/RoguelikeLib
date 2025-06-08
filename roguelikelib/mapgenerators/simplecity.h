#pragma once
#ifndef RL_SIMPLECITY_H
#define RL_SIMPLECITY_H

#include "../map.h"
#include "../randomness.h"
#include "../maputils.h"

namespace RL {
inline
void CreateSimpleCity(CMap &level, const int& a_number_of_buildings)
{
    const int min_building_width = 5;
    const int max_building_width = 10;
    const int min_building_height = 5;
    const int max_building_height = 10;

    if(level.GetWidth() == 0 || level.GetHeight() == 0) {
        return;
    }

    for(;;) { // until created with proper # of buildings
        level.Clear(LevelElementGrass);

        SRoom main;
        main.corner1.x = 0;
        main.corner1.y = 0;
        main.corner2.x = (int) level.GetWidth();
        main.corner2.y = (int) level.GetHeight();

        AddRecursiveRooms(level, LevelElementCorridor, max_building_width, max_building_height, main, false);

        int build_count = 0;

        int tries = 0;

        while(build_count != a_number_of_buildings && tries < 100) {
            int size_x = max_building_width * 2;
            int size_y = max_building_height * 2;

            while(1) {
                Position pos;

                if(FindOnMapRandomRectangleOfType(level, LevelElementGrass, pos, RL::Size(size_x + 2, size_y + 2))) {
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
                    DrawRectangleOnMap(level, building.corner1, building.corner2, LevelElementWall);
                    DrawRectangleOnMap(level, smaller.corner1, smaller.corner2, LevelElementRoom);
                    AddRecursiveRooms(level, LevelElementWall, 3, 3, smaller);

                    // add a doors leading out (improve to lead to nearest road)
                    if(CoinToss()) {
                        if(CoinToss()) {
                            level.SetCell(building.corner1.x + Random(size_x - 2) +1, building.corner1.y, LevelElementDoorClose);
                        } else {
                            level.SetCell(building.corner1.x + Random(size_x - 2) +1, building.corner2.y - 1, LevelElementDoorClose);
                        }
                    } else {
                        if(CoinToss()) {
                            level.SetCell(building.corner1.x, building.corner1.y + Random(size_y - 2) +1, LevelElementDoorClose);
                        } else {
                            level.SetCell(building.corner2.x - 1, building.corner1.y + Random(size_y - 2) +1, LevelElementDoorClose);
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
            for(int index = 0; index < level.GetWidth() * level.GetHeight() * 0.3; index++) {
                int x = Random(static_cast <int> (level.GetWidth()));
                int y = Random(static_cast <int> (level.GetHeight()));

                if(level.GetCell(x, y) == LevelElementGrass && CountNeighboursOfType(level, LevelElementWall, Position(x, y), true) == 0) {
                    level.SetCell(x, y, LevelElementPlant);
                }
            }

            return;
        }
    }
}

} // end of namespace RL

#endif

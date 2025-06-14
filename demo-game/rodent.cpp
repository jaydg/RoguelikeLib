#include "simple-game.h"
#include "rodent.h"
#include "../roguelikelib/pathfinding.h"

extern CSimpleGame game;

CRodent::CRodent()
{
    tile = 'r';
    hit_points = 5;
    strength = 3;
}

void CRodent::LookAround()
{
    CMonster::LookAround();

    if(fov.GetCell(game.player.GetPosition())) {
        enemy_pos = game.player.GetPosition();
    }
}

void CRodent::DoAction()
{
    LookAround();

    if(position != enemy_pos && game.level.OnMap(enemy_pos)) {
        std::vector<RL::Position> path;

        // Convert map to values for pathfinding
        RL::CMap temp_map = game.level;
        RL::Position pos;

        for(pos.x = 0; pos.x < LEVEL_SIZE_X; ++pos.x)
            for(pos.y = 0; pos.y < LEVEL_SIZE_Y; ++pos.y) {
                if(temp_map.GetCell(pos) != '#') {
                    const CMonster *monster = game.GetMonsterFromCell(pos);

                    // go around other monsters, don't go around player
                    if(monster != NULL && monster != &game.player && monster != this) {
                        temp_map.SetCell(pos, RL::LevelElementWall_value);
                    } else {
                        temp_map.SetCell(pos, RL::LevelElementCorridor_value);
                    }
                } else {
                    temp_map.SetCell(pos, RL::LevelElementWall_value);
                }
            }

        if(RL::FindPath(temp_map, position, enemy_pos, path)) {
            if(MoveTo(path[0])) {
                return;
            }
        }
    }
}

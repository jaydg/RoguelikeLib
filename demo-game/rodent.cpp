module;

export module demo_game.rodent:impl;

import demo_game.game;
import demo_game.monster;
import demo_game.rodent;
import rl.map;
import rl.pathfinding;
import rl.position;
import rl.randomness;
import std;

CRodent::CRodent()
{
    tile = 'r';
    // Random gray value (R=G=B) between 136 (0x88) and 255 (0xFF)
    std::uint32_t gray_val = 136 + RL::Random(120);
    rgb_color = (gray_val << 16) | (gray_val << 8) | gray_val;

    hit_points = 5;
    strength = 3;
}

void CRodent::LookAround()
{
    CMonster::LookAround();

    if(fov(game.player.GetPosition())) {
        enemy_pos = game.player.GetPosition();
    }
}

void CRodent::DoAction() {
    LookAround();

    if(position != enemy_pos && game.level.OnMap(enemy_pos)) {
        std::vector<RL::Position> path;

        // Convert map to values for pathfinding
        RL::CMap temp_map = game.level;
        RL::Position pos;

        for(pos.x = 0; pos.x < CSimpleGame::LEVEL_SIZE_X; ++pos.x)
            for(pos.y = 0; pos.y < CSimpleGame::LEVEL_SIZE_Y; ++pos.y) {
                if(temp_map.GetCell(pos) != '#') {
                    const CMonster *monster = game.GetMonsterFromCell(pos);

                    // go around other monsters, don't go around player
                    if(monster && monster != &game.player && monster != this) {
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

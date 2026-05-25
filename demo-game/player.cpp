module;

export module demo_game.player:impl;

import demo_game.game;
import demo_game.io;
import demo_game.monster;
import demo_game.player;
import rl.map;
import rl.position;
import rl.randomness;
import std;

CPlayer::CPlayer()
{
    tile = '@';
    rgb_color = 0xFFFF00;
    hit_points = 20;
    experience = 0;
    strength = 5;
    seen_map.Resize(CSimpleGame::LEVEL_SIZE_X, CSimpleGame::LEVEL_SIZE_Y);
    seen_map.Clear(0);
}

bool CPlayer::Attack(CMonster *monster)
{
    bool is_dead = CMonster::Attack(monster);

    if(is_dead) {
        GainExperience();
    }

    return is_dead;
}

void CPlayer::Regenerate()
{
    if(RL::Random(8) == 0 && hit_points < 20) {
        hit_points++;
    }
}

void CPlayer::GainExperience()
{
    experience++;
}

void CPlayer::DoAction() {
    LookAround();

    switch(IOGetKey()) {
    case '1':
        MoveTo(RL::Position(position.x - 1, position.y + 1));
        break;

    case '2':
        MoveTo(RL::Position(position.x, position.y + 1));
        break;

    case '3':
        MoveTo(RL::Position(position.x + 1, position.y + 1));
        break;

    case '4':
        MoveTo(RL::Position(position.x - 1, position.y));
        break;

    case '5':
        break;

    case '6':
        MoveTo(RL::Position(position.x + 1, position.y));
        break;

    case '7':
        MoveTo(RL::Position(position.x - 1, position.y - 1));
        break;

    case '8':
        MoveTo(RL::Position(position.x, position.y - 1));
        break;

    case '9':
        MoveTo(RL::Position(position.x + 1, position.y - 1));
        break;

    default:
        break;
    }
}

void CPlayer::Death()
{
    IOPrintString(30, 24, "You are dead!", 0xFF0000);
}

void CPlayer::Print() const
{
    CMonster::Print();
    IOPrintString(0, 24, "HP:   ");
    IOPrintValue(4, 24, hit_points);

    IOPrintString(10, 24, "EXP:");
    IOPrintValue(15, 24, experience);
}

void CPlayer::LookAround()
{
    CMonster::LookAround();

    // Print map
    RL::Position pos;

    for (pos.x = 0; pos.x < CSimpleGame::LEVEL_SIZE_X; ++pos.x)
    {
        for (pos.y = 0; pos.y < CSimpleGame::LEVEL_SIZE_Y; ++pos.y)
        {
            int cell = game.level.GetCell(pos);

            // Define colors for map elements
            // default: white
            std::uint32_t base_color = 0xFFFFFF;
            if (cell == RL::LevelElementWall)
            {
                // walls
                base_color = 0x8B4513;
            }
            else if (cell == RL::LevelElementRoom || cell == RL::LevelElementCorridor)
            {
                // floor
                cell = '.';
                base_color = 0xAAAAAA;
            }

            if (fov.GetCell(pos))
            {
                // currently visible
                seen_map.SetCell(pos, cell);

                // print with normal intensity
                IOPrintChar(pos.x, pos.y, cell, base_color);

                // paint visible monsters
                const CMonster* monster = game.GetMonsterFromCell(pos);

                if(monster != nullptr) {
                    monster->Print();
                }
            }
            else if(seen_map.GetCell(pos))
            {
                // known, but currently not visible
                // print the same character ('cell') with dimmed color
                std::uint8_t r = ((base_color >> 16) & 0xFF) * 0.35;
                std::uint8_t g = ((base_color >> 8) & 0xFF) * 0.35;
                std::uint8_t b = (base_color & 0xFF) * 0.35;
                std::uint32_t dark_color = (r << 16) | (g << 8) | b;

                IOPrintChar(pos.x, pos.y, cell, dark_color);
            }
            else
            {
                // unknown
                IOPrintChar(pos.x, pos.y, ' ');
            }
        }
    }

    IORefresh();
}

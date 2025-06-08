#include "simple-game.h"
#include "io.h"
#include "../roguelikelib/randomness.h"

extern CSimpleGame game;

CPlayer::CPlayer()
{
    tile = '@';
    hit_points = 20;
    experience = 0;
    strength = 5;
    seen_map.Resize(LEVEL_SIZE_X, LEVEL_SIZE_Y);
    seen_map.Clear(0);
}

void CPlayer::Regenerate()
{
    if(RL::Random(8) == 0 && hit_points < 20) {
        hit_points++;
    }
}

bool CPlayer::Attack(CMonster *monster)
{
    bool is_dead = CMonster::Attack(monster);

    if(is_dead) {
        GainExperience();
    }

    return is_dead;
}

void CPlayer::GainExperience()
{
    experience++;
}

void CPlayer::DoAction()
{
    LookAround();
    int key = IOGetKey();

    switch(key) {
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
    }
}

void CPlayer::Death()
{
    IOPrintString(30, 24, "You are dead!");
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

    for(pos.x = 0; pos.x < LEVEL_SIZE_X; ++pos.x) {
        for(pos.y = 0; pos.y < LEVEL_SIZE_Y; ++pos.y) {
            if(fov.GetCell(pos)) { // if visible
                int cell = game.level.GetCell(pos);
                seen_map.SetCell(pos, cell);

                if(cell != '#') {
                    cell = '.';
                }

                IOPrintChar(pos.x, pos.y, cell);
                const CMonster* monster = game.GetMonsterFromCell(pos);

                if(monster != NULL) {
                    monster->Print();
                }
            } else if(seen_map.GetCell(pos)) {
                int seen_cell = seen_map.GetCell(pos);

                if(seen_cell == '#') {
                    IOPrintChar(pos.x, pos.y, '%');
                } else {
                    IOPrintChar(pos.x, pos.y, ' ');
                }
            } else {
                IOPrintChar(pos.x, pos.y, ' ');
            }
        }
    }

    IORefresh();
}

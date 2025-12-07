#include "simple-game.h"
#include "io.h"
#include "../roguelikelib/randomness.h"
#include "../roguelikelib/fov.h"

extern CSimpleGame game;

bool CMonster::Damage(int damage) // returns true if enemy died
{
    hit_points -= damage;

    if(hit_points <= 0) {
        Death();
        return true;
    }

    return false;
}

bool CMonster::Attack(CMonster *monster)
{
    return monster->Damage(RL::Random(strength));
}

void CMonster::Print() const
{
    IOPrintChar(position.x, position.y, tile);
}

void CMonster::LookAround()
{
    fov = game.level;

    for(int x = 0; x < LEVEL_SIZE_X; ++x)
        for(int y = 0; y < LEVEL_SIZE_Y; ++y) {
            fov.SetCell(x, y, fov.GetCell(x, y) == RL::LevelElementWall); // blocked only if Wall
        }

    RL::CalculateFOV(fov, position, 5);
}

void CMonster::Death()
{
    // inform the game that monster died
    game.monsters_to_remove.insert(this);
    position.x = -1;
    position.y = -1;
};

RL::Position CMonster::GetPosition() const
{
    return position;
}

bool CMonster::MoveTo(const RL::Position &new_pos)
{
    int cell = game.level.GetCell(new_pos);

    if(cell != -1 && cell != '#') {
        // if monster there
        CMonster *monster = game.GetMonsterFromCell(new_pos);

        if(monster == nullptr) {
            position = new_pos;
            return true;
        }

        if(monster != this) {
            Attack(monster);
        }

        return false;
    }

    return false;
}

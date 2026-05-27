module;

export module demo_game.monster:impl;

import demo_game.game;
import demo_game.io;
import demo_game.monster;
import rl.randomness;
import rl.tile;

bool CMonster::Attack(CMonster *monster)
{
    return monster->Damage(RL::Random(strength));
}

void CMonster::LookAround()
{
    fov = RL::CFOV(&game.level);
    fov.Calculate(position, 5);
}

bool CMonster::MoveTo(const RL::Position &new_pos)
{
    RL::CTile cell = game.level.GetCell(new_pos);

    if(cell.isPassable()) {
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

bool CMonster::Damage(int damage)
{
    hit_points -= damage;

    if(hit_points <= 0) {
        Death();
        return true;
    }

    return false;
}

void CMonster::Death()
{
    // inform the game that monster died
    game.monsters_to_remove.insert(this);
    position.x = -1;
    position.y = -1;
}

void CMonster::Print() const
{
    IOPrintChar(position.x, position.y, tile, rgb_color);
}

RL::Position CMonster::GetPosition() const
{
    return position;
}

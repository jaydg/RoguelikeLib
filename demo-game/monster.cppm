module;

export module demo_game.monster;

import rl.map;
import rl.position;

export class CMonster
{
protected:
    char tile{};
    RL::CMap fov;
    int hit_points{};
    int strength{};
    RL::Position position;
public:
    virtual ~CMonster() = default;

    virtual void DoAction() = 0;
    virtual bool Attack(CMonster *monster);
    virtual void LookAround();
    virtual bool MoveTo(const RL::Position &new_pos);

    // return true if enemy died
    virtual bool Damage(int damage);

    virtual void Death();
    virtual void Print() const;
    RL::Position GetPosition() const;
};

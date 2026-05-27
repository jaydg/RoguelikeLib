module;

export module demo_game.monster;

export import rl.fov;
export import rl.map;
export import rl.position;
import std;

export class CMonster
{
protected:
    char tile{};
    std::uint32_t rgb_color{0xFFFFFF};
    RL::CFOV fov;
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

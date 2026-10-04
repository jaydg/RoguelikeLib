module;

export module demo_game.monster;

export import rl.fov;
export import rl.map;
export import rl.position;
export import rl.scheduler;
import std;

export class CMonster
{
protected:
    char32_t tile{};
    std::uint32_t rgb_color{0xFFFFFF};
    RL::CFOV fov;
    int hit_points{};
    int strength{};

    // How quick it is: 100 is as quick as the player, 200 twice as quick
    int speed{100};

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

    // How long until it acts again after acting: 100 ticks at speed 100
    RL::Ticks Delay() const;
};

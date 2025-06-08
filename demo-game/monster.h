#include "../roguelikelib/map.h"

class CMonster {
protected:
    char tile;
    RL::CMap fov;
    int hit_points;
    int strength;
    RL::Position position;
public:
    ~CMonster() {};

    virtual void DoAction() = 0;
    virtual bool Attack(CMonster *monster);
    virtual void LookAround();
    virtual bool MoveTo(const RL::Position &new_pos);
    virtual bool Damage(int damage); // return true if enemy died
    virtual void Death();
    virtual void Print() const;
    RL::Position GetPosition() const;
};

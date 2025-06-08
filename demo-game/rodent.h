#include "../roguelikelib/map.h"

class CRodent : public CMonster {
private:
    RL::Position enemy_pos;
public:
    CRodent();
    virtual void LookAround();
    virtual void DoAction();
};

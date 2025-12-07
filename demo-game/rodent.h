#include "../roguelikelib/map.h"

class CRodent : public CMonster {
private:
    RL::Position enemy_pos;
public:
    CRodent();
    void LookAround() override;
    void DoAction() override;
};

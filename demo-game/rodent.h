#ifndef DEMO_GAME_RODENT_H
#define DEMO_GAME_RODENT_H

#include "monster.h"

class CRodent : public CMonster {
private:
    RL::Position enemy_pos;
public:
    CRodent();
    void LookAround() override;
    void DoAction() override;
};

#endif

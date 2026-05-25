module;

export module demo_game.rodent;

import demo_game.monster;
import rl.position;

export class CRodent : public CMonster {
private:
    RL::Position enemy_pos;
public:
    CRodent();
    void LookAround() override;
    void DoAction() override;
};

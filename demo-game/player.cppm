module;

export module demo_game.player;

import demo_game.monster;
import rl.map;

export class CPlayer final : public CMonster {
private:
    int experience;
    RL::CMap seen_map;
public:
    CPlayer();
    bool Attack(CMonster *monster) override;
    void Regenerate();
    void GainExperience();
    void DoAction() override;
    void Death() override;
    void Print() const override;
    void LookAround() override;
};

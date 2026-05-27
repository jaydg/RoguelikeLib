module;

export module demo_game.player;

import demo_game.monster;
export import rl.matrix;

export class CPlayer final : public CMonster {

private:

    int experience;
    RL::CMatrix<bool> seen_map;

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

#include "monster.h"

class CPlayer : public CMonster {
private:
    int experience;
    RL::CMap seen_map;
public:
    CPlayer();
    virtual bool Attack(CMonster *monster) override;
    virtual void Regenerate();
    virtual void GainExperience();
    virtual void DoAction() override;
    virtual void Death() override;
    virtual void Print() const override;
    virtual void LookAround() override;
};

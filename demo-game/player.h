#include "monster.h"

class CPlayer : public CMonster {
private:
	int experience;
	RL::CMap seen_map;
public:
	CPlayer();
	virtual bool Attack(CMonster *monster);
	virtual void Regenerate();
	virtual void GainExperience();
	virtual void DoAction();
	virtual void Death();
	virtual void Print();
	virtual void LookAround();
};
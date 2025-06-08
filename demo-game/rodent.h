#include "../include/roguelikelib.h"

class CRodent : public CMonster {
private:
	RL::SPosition enemy_pos;
public:
	CRodent();
	virtual void LookAround();
	virtual void DoAction();
};
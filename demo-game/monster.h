#include "../include/roguelikelib.h"

class CMonster {
public:
	RL::CMap fov;
	char tile;
	int hit_points;
	int strength;
	RL::SPosition position;
	virtual void DoAction()=0;
	virtual bool Attack(CMonster *monster);
	virtual void LookAround();
	virtual bool MoveTo(const RL::SPosition &new_pos);
	virtual bool Damage(int damage); // return true if enemy died
	virtual void Death();
	virtual void Print();
};
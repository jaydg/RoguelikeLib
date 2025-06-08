/*!
 * Simple game using RoguelikeLib
 * This example shows all the main features of the RoguelikeLib in a simple real game
 * - Map management and generation
 * - Randomness
 * - FOV calculation
 * - Path finding
 *
 * This game can be a base for your roguelike.
 * The source code is very unoptimized, however "Premature optimization is the root of all evil".
 *
 * To compile you need a PDCurses library (for standard project placed in pdcurses subdirectory)
 * http://pdcurses.sourceforge.net
 */

#include <list>
#include <set>
#include "simple-game.h"
#include "io.h"
#include "rodent.h"

// game is global
CSimpleGame game;

void CSimpleGame::PlacePlayer()
{
	RL::SPosition pos(0,0);
	if (RL::FindOnMapRandomRectangleOfType(level,RL::LevelElementRoom,pos,1,1))
		player.position=pos;
	else if (RL::FindOnMapRandomRectangleOfType(level,RL::LevelElementCorridor,pos,1,1))
		player.position=pos;

	monsters.push_back(&player);
}

void CSimpleGame::AddMonster()
{
	CRodent *new_one = new CRodent;
	RL::SPosition pos(0,0);
	if (RL::FindOnMapRandomRectangleOfType(level,RL::LevelElementRoom,pos,1,1))
		new_one->position=pos;
	else if (RL::FindOnMapRandomRectangleOfType(level,RL::LevelElementCorridor,pos,1,1))
		new_one->position=pos;

	monsters.push_back(new_one);
}

void CSimpleGame::MoveAllMonsters()
{
	std::list < CMonster * >::iterator it, _it;
	for (it=monsters.begin(),_it=monsters.end();it!=_it;++it)
	{
		CMonster *monster = *it;
		if (monsters_to_remove.find(monster)==monsters_to_remove.end())
			monster->DoAction();
	}
	// remove all dead monsters
	std::list < CMonster * >::iterator to_remove;

	for (it=monsters.begin(),_it=monsters.end();it!=_it;)
	{
		to_remove=it;
		it++;
		CMonster *monster = *to_remove;
		if (monsters_to_remove.find(monster)!=monsters_to_remove.end())
		{
			delete monster;
			monsters.erase(to_remove);
		}
	}
	monsters_to_remove.clear();
}

void CSimpleGame::Start()
{
	level.Resize(LEVEL_SIZE_X,LEVEL_SIZE_Y);

	int level_type=RL::Random(6);
	switch(level_type)
	{
	case 0:
		RL::CreateStandardDunegon(level,20,false);
		IOPrintString(60,24,"Standard Dungeon");
		break;
	case 1:
		RL::CreateAntNest(level,true);
		IOPrintString(60,24,"Ant Nest");
		break;
	case 2:
		RL::CreateMines(level,12);
		IOPrintString(60,24,"Mines");
		break;
	case 3:
		RL::CreateCaves(level,2);
		IOPrintString(60,24,"Caves");
		break;
	case 4:
		RL::CreateMaze(level,false);
		IOPrintString(60,24,"Maze");
		break;
	case 5:
		RL::CreateSpaceShuttle(level);
		IOPrintString(60,24,"Space Shuttle");
		break;
	}

	//////////////////////////////////////////////////////////////////////////

	PlacePlayer();
	for (int index=0;index<15;++index)
		AddMonster();
	for (;;) // next turn
	{
		if (RL::Random(100)==0)
			AddMonster();

		player.Regenerate();
		MoveAllMonsters();
	}
}

bool CMonster::MoveTo(const RL::SPosition &new_pos)
{
	int cell = game.level.GetCell(new_pos);
	if (cell!=-1 && cell!='#')
	{
		// if monster there
		CMonster *monster = game.GetMonsterFromCell(new_pos);
		if (monster==NULL)
		{
			position=new_pos;
			return true;
		}
		if (monster!=this)
			Attack(monster);
		return false;
	}
	return false;
}

void CPlayer::LookAround()
{
	CMonster::LookAround();

	// Print map
	RL::SPosition pos;
	for (pos.x=0;pos.x<LEVEL_SIZE_X;++pos.x)
	{
		for (pos.y=0;pos.y<LEVEL_SIZE_Y;++pos.y)
		{
			if (fov.GetCell(pos)) // if visible
			{
				int cell = game.level.GetCell(pos);
				seen_map.SetCell(pos,cell);
				if (cell!='#')
					cell='.';
				IOPrintChar(pos.x,pos.y,cell);
				CMonster *monster = game.GetMonsterFromCell(pos);
				if (monster!=NULL)
					monster->Print();
			}
			else if (seen_map.GetCell(pos))
			{
				int seen_cell = seen_map.GetCell(pos);
				if (seen_cell=='#')
					IOPrintChar(pos.x,pos.y,'%');				
				else
					IOPrintChar(pos.x,pos.y,' ');				
			}
			else
			{
				IOPrintChar(pos.x,pos.y,' ');				
			}
		}
	}
	IORefresh();
}


CMonster *CSimpleGame::GetMonsterFromCell(const RL::SPosition &cell)
{
	std::list < CMonster * >::iterator it, _it;
	for (it=game.monsters.begin(),_it=game.monsters.end();it!=_it;++it)
	{
		CMonster *monster = *it;		
		if (monster->position==cell)
			return monster;
	}
	return NULL;
}

int main(void)
{	
	IOInit();
	RL::InitRandomness();
	game.Start();
	return 0;
}

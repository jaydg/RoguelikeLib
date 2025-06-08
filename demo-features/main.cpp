#include "../include/RoguelikeLib.h"

int main(void)
{
	RL::InitRandomness();
	RL::CMap level;

	const int level_size_x=79; 
	const int level_size_y=50;

	level.Resize(level_size_x,level_size_y);
	cout << endl << "Standard Dungeon" << endl << endl;
	RL::CreateStandardDunegon(level,20);
	level.PrintMap();

	cout << endl << "Ant's Nest" << endl << endl;
	RL::CreateAntNest(level,true);
	level.PrintMap();

	cout << endl << "Mines" << endl << endl;
	RL::CreateMines(level,25);
	level.PrintMap();

	cout << endl << "Caves Sharp" << endl << endl;
	RL::CreateCaves(level,1,0.75);
	level.PrintMap();

	cout << endl << "Caves Soft" << endl << endl;
	RL::CreateCaves(level,3);
	level.PrintMap();

	cout << endl << "Space shuttle" << endl << endl;
	RL::CreateSpaceShuttle(level,25);
	level.PrintMap();

	cout << endl << "Castle" << endl << endl;
	RL::CreateSpaceShuttle(level,25,true);
	level.PrintMap();

	cout << endl << "Simple City with 15 buildings" << endl << endl;
	RL::CreateSimpleCity(level,15);
	level.PrintMap();

	cout << endl << "Field of View in Simple City from the road" << endl << endl;

	RL::SPosition observer(level_size_x/2,level_size_y/2);
	// Place observer somewhere on a horizontal road
	RL::FindOnMapRandomRectangleOfType(level,RL::LevelElementCorridor,observer,2,1);	

	RL::CMap fov;
	fov.Resize(level_size_x,level_size_y);

	RL::SPosition pos;
	for (pos.x=0;pos.x<level_size_x;++pos.x)
	{
		for (pos.y=0;pos.y<level_size_y;++pos.y)
		{
			if (level.GetCell(pos)==RL::LevelElementWall || level.GetCell(pos)==RL::LevelElementDoorClose)
				fov.SetCell(pos,1); // blocks
			else
				fov.SetCell(pos,0); // doesn't block
		}
	}
	RL::CalculateFOV(fov,observer,80);
	for (pos.y=0;pos.y<level_size_y;++pos.y)
	{
		for (pos.x=0;pos.x<level_size_x;++pos.x)
		{
			if (pos==observer)
				cout << '@';
			else if (fov.GetCell(pos)==1)
				cout << (char) level.GetCell(pos);
			else
				cout << ' ';
		}
		cout << endl;
	}
	
	cout << endl << "Maze" << endl << endl;
	RL::CreateMaze(level);
	level.PrintMap();

	cout << endl << "Path in this maze '+' (from top-left to bottom-right corner)" << endl << endl;

	RL::CMap temp_level=level;
	RL::SPosition start, end;
	// convert tiles to values and find corners at the same time

	for (pos.x=0;pos.x<level_size_x;++pos.x)
	{
		for (pos.y=0;pos.y<level_size_y;++pos.y)
		{
			if (temp_level.GetCell(pos)==RL::LevelElementCorridor)
			{
				temp_level.SetCell(pos,RL::LevelElementCorridor_value);
				// set top-left corner
				if (start.x==-1)
					start = pos;
				// set bottom-right corner
				end = pos;
			}
			else
				temp_level.SetCell(pos,RL::LevelElementWall_value);
		}
	}

	// find path in maze

	vector < RL::SPosition > path;
	RL::FindPath(temp_level,start,end,path);

	// print maze with path

	for (size_t index=0;index<path.size();index++)
		level.SetCell(path[index].x,path[index].y,'+');
	level.PrintMap();
	return 0;
}


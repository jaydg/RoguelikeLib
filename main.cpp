#include <time.h>
#include "RoguelikeLib.h"

int main(void)
{
	srand((unsigned int) time(0));

	CRoguelikeLibrary rl;

	const int level_size_x=79; 
	const int level_size_y=50;

	rl.ResizeLevel(level_size_x,level_size_y);

	cout << endl << "Standard Dungeon" << endl << endl;
	level_t &level = rl.CreateStandardDunegon(20);
	rl.PrintLevel();


	cout << endl << "Ant's Nest" << endl << endl;
	level = rl.CreateAntNest(true);
	rl.PrintLevel();

	cout << endl << "Mines" << endl << endl;
	level = rl.CreateMines(25);
	rl.PrintLevel();

	cout << endl << "Caves Sharp" << endl << endl;
	level = rl.CreateCaves(1,0.75);
	rl.PrintLevel();

	cout << endl << "Caves Soft" << endl << endl;
	level = rl.CreateCaves(3);
	rl.PrintLevel();

	cout << endl << "Space shuttle" << endl << endl;
	level = rl.CreateSpaceShuttle(25);
	rl.PrintLevel();

	cout << endl << "Castle" << endl << endl;
	level = rl.CreateSpaceShuttle(25,true);
	rl.PrintLevel();

	cout << endl << "Maze" << endl << endl;
	level = rl.CreateMaze();
	rl.PrintLevel();

	cout << endl << "Path in this maze '+' (from top-left to bottom-right corner)" << endl << endl;

	// store generated maze
	level_t generated_maze=level;

	RL::SPosition start, end;

	// convert tiles to values and find corners
	for (int x=0;x<level_size_x;++x)
		for (int y=0;y<level_size_y;++y)
		{
			if (level[x][y]==RL::Corridor)
			{
				level[x][y]=RL::Corridor_value;
				// set top-left corner
				if (start.x==-1)
				{
					start.x=x;
					start.y=y;
				}
				// set bottom-right corner
				end.x=x;
				end.y=y;
			}
			else
				level[x][y]=RL::Wall_value;
		}

	// find path in maze

	vector < RL::SPosition > path;
	rl.FindPath(start,end,path);

	// print maze with path

	for (size_t index=0;index<path.size();index++)
		generated_maze[path[index].x][path[index].y]='+';
	rl.SetLevel(generated_maze);
	rl.PrintLevel();
	return 0;
}
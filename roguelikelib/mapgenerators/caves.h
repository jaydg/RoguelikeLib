#pragma once
#ifndef RL_CAVES_H
#define RL_CAVES_H

#include "../map.h"
#include "../randomness.h"
#include "../maputils.h"

namespace RL {
	inline 
	void CreateCaves(CMap &level,int iterations=1, float density=0.65)
	{
		if (level.GetWidth()==0 || level.GetHeight()==0)
			return;

		level.Clear(LevelElementRoom);

		// create a game of life cave

		int x,y;

		for(int fill=0; fill<(level.GetWidth()*level.GetHeight()*density); fill++)
			level.SetCell(Random((int) level.GetWidth()),Random((int) level.GetHeight()),LevelElementWall);

		for(int iteration=0; iteration<iterations; iteration++)
		{
			for(x=0; x<(int) level.GetWidth(); x++)
			{
				for(y=0; y<(int) level.GetHeight(); y++)
				{
					int neighbours=CountNeighboursOfType(level,LevelElementWall,Position(x,y));

					if (level.GetCell(x,y)==LevelElementWall)
					{
						if (neighbours<4)
							level.SetCell(x,y,LevelElementRoom);
					}
					else
					{
						if (neighbours>4)
							level.SetCell(x,y,LevelElementWall);
					}

					if (x==0 || x==(int) level.GetWidth()-1 || y==0 || y==(int) level.GetHeight()-1)
						level.SetCell(x,y,LevelElementWall);
				}
			}
		}

		ConnectClosestRooms(level,true);
		ConvertValuesToTiles(level);
	};

} // end of namespace RL

#endif

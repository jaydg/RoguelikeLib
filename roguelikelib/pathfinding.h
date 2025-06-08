//////////////////////////////////////////////////////////////////////////
// Path finding
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_PATHFINDING_H
#define RL_PATHFINDING_H

#include "map.h"
#include "maputils.h"

namespace RL {
	// CMap &level - in/out 
	// input level must be in form of values (higher than floodfill distance)
	// - LevelElementCorridor_value
	// - LevelElementWall_value
	// as output you get a flood filled level
	bool FindPath(CMap &level, const RL::Position& start, const RL::Position& end, std::vector < RL::Position >& path, const bool& diagonals=true)
	{
		// fill from end to start
		if (!FloodFill(level, end,0,false,1,start))
			return false;

		// walk from start to end
		Position pos=start;
		Position new_pos=start;

		while(1)
		{
			if (pos==end)
				return true;
			pos = new_pos;

			if (pos!=start)
				path.push_back(pos);

			int current_value = level.GetCell(pos.x,pos.y);

			if (diagonals)
			{
				if (pos.x>0 && pos.y>0)
					if (level.GetCell(pos.x-1,pos.y-1)<current_value)  // NW
					{
						new_pos.x--;     
						new_pos.y--;     
						continue;
					}

					if (pos.x<(int) level.GetWidth()-1 && pos.y>0)
						if (level.GetCell(pos.x+1,pos.y-1)<current_value) // NE
						{
							new_pos.x++;     
							new_pos.y--;     
							continue;
						}

						if (pos.x<(int) level.GetWidth()-1 && pos.y<(int) level.GetHeight()-1) // SE
							if (level.GetCell(pos.x+1,pos.y+1)<current_value) 
							{
								new_pos.x++;     
								new_pos.y++;     
								continue;
							}

							if (pos.x>0 && pos.y<(int) level.GetHeight()-1)
								if (level.GetCell(pos.x-1,pos.y+1)<current_value) // SW
								{
									new_pos.x--;     
									new_pos.y++;     
									continue;
								}
			}

			if (pos.y>0)
				if (level.GetCell(pos.x,pos.y-1)<current_value) // N
				{
					new_pos.y--;     
					continue;
				}

				if (pos.x<(int) level.GetWidth()-1)
					if (level.GetCell(pos.x+1,pos.y)<current_value) // E
					{
						new_pos.x++;     
						continue;
					}

					if (pos.x>0 && pos.y<(int) level.GetHeight()-1)
						if (level.GetCell(pos.x,pos.y+1)<current_value) // S
						{
							new_pos.y++;     
							continue;
						}

						if (pos.x>0 && pos.y>0)
							if (level.GetCell(pos.x-1,pos.y)<current_value) // W
							{
								new_pos.x--;     
								continue;
							}
							if (pos==new_pos && new_pos!=end)
							{
								path.clear();
								return false;
							}
		}
	}

}  // end of namespace RL

#endif

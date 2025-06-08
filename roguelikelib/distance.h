//////////////////////////////////////////////////////////////////////////
// Distance calculation
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_DISTANCE_H
#define RL_DISTANCE_H

#include "map.h"

namespace RL {
	//////////////////////////////////////////////////////////////////////////
	// To speed up distance calculation
	static std::vector <int> square_root;
	//////////////////////////////////////////////////////////////////////////

	static inline
	void InitSquareRoot(int size)
	{
		size_t old_size = square_root.size();

		if (size<0 || static_cast <size_t>(size)<old_size)
			return;

		size++;
		square_root.resize(size);

		size_t size2 = square_root.size();

		// count square root
		for (size_t a=old_size;a<static_cast <size_t>(size);++a)
			square_root[a]=(int )ceil(sqrt((double) a));				
	}

	static inline
	int Distance(const int& x1,const int& y1,const int& x2,const int& y2)
	{
		const int diff_x = x2-x1;
		const int diff_y = y2-y1;
		int dist = diff_x*diff_x + diff_y*diff_y;

		if (dist>=static_cast <int> (square_root.size()))
			InitSquareRoot(dist);

		return square_root[dist];
	}

	static inline
	int Distance(const Position &p1, const Position &p2)
	{
		return Distance(p1.x,p1.y,p2.x,p2.y);
	}


} // end of namespace RL

#endif

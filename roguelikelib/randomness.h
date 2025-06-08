//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_RANDOMNESS_H
#define RL_RANDOMNESS_H

#include "extern/mt19937int.h"
#include <time.h>

namespace RL {
	inline
	void InitRandomness(void)
	{
		init_genrand((unsigned int) time(0));
	};

	inline
	int Random(int value)
	{ 
		int random_value;
		if (value==0)
			return 0;
		random_value= (int) (((float)genrand_int32() / (float)0xFFFFFFFF)*(value));
		return random_value;
	};

	inline
	bool RandomLowerThatLimit(int limit, int value)
	{
		if (value==0)
			return false;
		if (Random(value)<limit)
			return true;
		return false;
	}

	inline
	bool CoinToss()
	{ 
		return Random(2)!=0; 
	};
}

#endif

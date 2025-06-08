//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_RANDOMNESS_H
#define RL_RANDOMNESS_H

#include <ctime>
#include <random>

namespace RL {

static std::mt19937 mt;

inline
void InitRandomness(void)
{
    mt.seed((unsigned int) time(nullptr));
}

inline
int Random(int value)
{
    int random_value;

    if(value == 0) {
        return 0;
    }

    random_value = (int)(((float)mt() / (float)0xFFFFFFFF) * (value));
    return random_value;
}

inline
bool RandomLowerThatLimit(int limit, int value)
{
    if(value == 0) {
        return false;
    }

    if(Random(value) < limit) {
        return true;
    }

    return false;
}

inline
bool CoinToss()
{
    return Random(2) != 0;
}

template <class RandomAccessIterator>
void Shuffle(RandomAccessIterator first, RandomAccessIterator last)
{
    for(auto i = (last - first) -1; i > 0; --i) {
        std::uniform_int_distribution<decltype(i)> d(0, i);
        swap(first[i], first[d(mt)]);
    }
}

}

#endif

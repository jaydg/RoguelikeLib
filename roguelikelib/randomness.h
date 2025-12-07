//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_RANDOMNESS_H
#define RL_RANDOMNESS_H

#include <random>

namespace RL {

static std::mt19937 mt;

inline
void InitRandomness()
{
    std::random_device rd; // non-deterministic generator
    mt.seed(rd());         // to seed mersenne twister.
}

inline
unsigned Random(const size_t limit)
{
    if(limit == 0) {
        return 0;
    }

    std::uniform_int_distribution<size_t> dist(0,limit - 1);
    return dist(mt);
}

inline
bool RandomLowerThatLimit(const size_t limit, const size_t value)
{
    if(value == 0) {
        return false;
    }

    return Random(value) < limit;
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

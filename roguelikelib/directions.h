//////////////////////////////////////////////////////////////////////////
// Directions
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_DIRECTIONS_H
#define RL_DIRECTIONS_H

#include "randomness.h"

namespace RL {
enum EDirection {
    EDirectionMin = 0,
    North = EDirectionMin,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
    EDirectionMax = NorthWest
};

class Direction {
    EDirection direction;
public:
    Direction(): direction(North) {};

    explicit Direction(const EDirection start): direction(start) {};

    EDirection Get() const
    {
        return direction;
    }

    void Set(const EDirection to_set)
    {
        direction = to_set;
    }
};

inline
EDirection operator++(EDirection & e, int)
{
    e = (e < EDirectionMax) ? static_cast<EDirection>(e + 1) : EDirectionMin;
    return e;
}

inline
EDirection operator--(EDirection & e, int)
{
    e = (e > EDirectionMin) ? static_cast<EDirection>(e - 1) : EDirectionMax;
    return e;
}

inline
EDirection RandomDirection()
{
    return static_cast<EDirection>(Random(EDirectionMax));
}

} // end of namespace RL

#endif

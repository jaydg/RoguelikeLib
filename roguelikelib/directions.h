//////////////////////////////////////////////////////////////////////////
// Directions
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_DIRECTIONS_H
#define RL_DIRECTIONS_H

#include "roguelikelib.h"
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
private:
    EDirection direction;
public:
    Direction(): direction(North) {};

    Direction(EDirection start): direction(start) {};

    inline EDirection Get()
    {
        return direction;
    }

    inline void Set(EDirection to_set)
    {
        direction = to_set;
    }
};

inline
EDirection operator++(EDirection & e, int)
{
    e = ((e < EDirectionMax) ? (EDirection)(e + 1) : EDirectionMin) ;
    return e;
}

inline
EDirection operator--(EDirection & e, int)
{
    e = ((e > EDirectionMin) ? (EDirection)(e - 1) : EDirectionMax) ;
    return e;
}

inline
EDirection RandomDirection()
{
    return (EDirection) Random(EDirectionMax);
}

} // end of namespace RL

#endif

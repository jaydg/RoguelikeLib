//////////////////////////////////////////////////////////////////////////
// Directions
//////////////////////////////////////////////////////////////////////////

export module rl.directions;

import rl.randomness;

export namespace RL {

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
    Direction() : direction(North) {}

    explicit Direction(const EDirection start) : direction(start) {}

    [[nodiscard]] EDirection Get() const
    {
        return direction;
    }

    void Set(const EDirection to_set)
    {
        direction = to_set;
    }
};

EDirection operator++(EDirection& e, int)
{
    e = (e < EDirectionMax) ? static_cast<EDirection>(e + 1) : EDirectionMin;
    return e;
}

EDirection operator--(EDirection& e, int)
{
    e = (e > EDirectionMin) ? static_cast<EDirection>(e - 1) : EDirectionMax;
    return e;
}

EDirection RandomDirection()
{
    return static_cast<EDirection>(Random(EDirectionMax));
}

} // end of namespace RL

//////////////////////////////////////////////////////////////////////////
// Directions
//////////////////////////////////////////////////////////////////////////

export module rl.directions;

import rl.randomness;
import std;

export namespace RL
{

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

class Direction
{
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

/**
 * @brief How a step in a direction moves, on a map with y growing downwards.
 *
 * @param direction The direction of the step.
 * @return The change of x and of y, each -1, 0 or 1: North is (0, -1), SouthEast (1, 1).
 */
[[nodiscard]] std::pair<int, int> DirectionDelta(EDirection direction)
{
    switch (direction) {
    case North:
        return {0, -1};

    case NorthEast:
        return {1, -1};

    case East:
        return {1, 0};

    case SouthEast:
        return {1, 1};

    case South:
        return {0, 1};

    case SouthWest:
        return {-1, 1};

    case West:
        return {-1, 0};

    case NorthWest:
    default:
        return {-1, -1};
    }
}

} // end of namespace RL

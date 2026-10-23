/**
 * @file directions.cppm
 * @brief 8-way compass directions, cyclic rotation operators, and coordinate delta mappings.
 */

module;

//////////////////////////////////////////////////////////////////////////
// Directions
//////////////////////////////////////////////////////////////////////////

export module rl.directions;

import rl.randomness;
import std;

export namespace RL
{

/**
 * @brief 8-way discrete compass directions arranged in clockwise order.
 *
 * Indexed from 0 (@ref North) through 7 (@ref NorthWest).
 * Supports cyclic traversal using @ref operator++(EDirection&, int) and
 * @ref operator--(EDirection&, int).
 */
enum EDirection {
    EDirectionMin = 0,         ///< Minimum direction enum value (@ref North).
    North = EDirectionMin,     ///< North: step (0, -1) towards decreasing y.
    NorthEast,                 ///< North-East: step (1, -1).
    East,                      ///< East: step (1, 0) towards increasing x.
    SouthEast,                 ///< South-East: step (1, 1).
    South,                     ///< South: step (0, 1) towards increasing y.
    SouthWest,                 ///< South-West: step (-1, 1).
    West,                      ///< West: step (-1, 0) towards decreasing x.
    NorthWest,                 ///< North-West: step (-1, -1).
    EDirectionMax = NorthWest  ///< Maximum direction enum value (@ref NorthWest).
};

/**
 * @brief State container wrapping an @ref EDirection value.
 *
 * Provides orientation state tracking, initialization, retrieval, and modification.
 */
class Direction
{
    EDirection direction;
public:
    /**
     * @brief Constructs a default direction facing @ref North.
     */
    Direction() : direction(North) {}

    /**
     * @brief Constructs a direction initialized with the given compass orientation.
     * @param start Initial compass direction.
     */
    explicit Direction(const EDirection start) : direction(start) {}

    /**
     * @brief Retrieves the current compass direction.
     * @return The held @ref EDirection value.
     */
    [[nodiscard]] EDirection Get() const
    {
        return direction;
    }

    /**
     * @brief Sets the compass direction to a new orientation.
     * @param to_set The new @ref EDirection value to set.
     */
    void Set(const EDirection to_set)
    {
        direction = to_set;
    }
};

/**
 * @brief Postfix increment operator to rotate a direction clockwise by 45 degrees.
 *
 * Cycles clockwise through the 8 compass directions. If the value reaches @ref EDirectionMax
 * (@ref NorthWest), it wraps around to @ref EDirectionMin (@ref North).
 *
 * @param e Reference to the direction being incremented.
 * @return The original direction value prior to the increment.
 */
EDirection operator++(EDirection& e, int)
{
    e = (e < EDirectionMax) ? static_cast<EDirection>(e + 1) : EDirectionMin;
    return e;
}

/**
 * @brief Postfix decrement operator to rotate a direction counter-clockwise by 45 degrees.
 *
 * Cycles counter-clockwise through the 8 compass directions. If the value reaches @ref EDirectionMin
 * (@ref North), it wraps around to @ref EDirectionMax (@ref NorthWest).
 *
 * @param e Reference to the direction being decremented.
 * @return The original direction value prior to the decrement.
 */
EDirection operator--(EDirection& e, int)
{
    e = (e > EDirectionMin) ? static_cast<EDirection>(e - 1) : EDirectionMax;
    return e;
}

/**
 * @brief Generates a pseudo-random 8-way compass direction.
 *
 * Uses @ref Random() to select an orientation uniformly distributed from
 * @ref EDirectionMin up to @ref EDirectionMax.
 *
 * @return A randomly selected @ref EDirection value.
 */
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

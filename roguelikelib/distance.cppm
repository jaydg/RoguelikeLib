/**
 * @file distance.cppm
 * @brief 2D discrete distance calculations and geometric metric functions.
 */

module;

//////////////////////////////////////////////////////////////////////////
// Distance calculation
//////////////////////////////////////////////////////////////////////////

export module rl.distance;

import std;

namespace RL
{

namespace Detail
{
    /**
     * @brief Computes the integer ceiling square root of an integer: @f$ \lceil \sqrt{n} \rceil @f$.
     *
     * Uses a Newton-Raphson iteration with initial bit estimation from leading zero count.
     * Evaluated at compile time to support constexpr Euclidean distance calculation.
     *
     * @param n Unsigned 64-bit integer whose ceiling square root is computed.
     * @return Smallest integer @f$ r @f$ such that @f$ r^2 \ge n @f$.
     */
    [[nodiscard]] constexpr std::size_t CeilISqrt(std::size_t n) noexcept
    {
        if (n <= 1) {
            return n;
        }
        std::size_t x0 = std::size_t{1} << ((64 - std::countl_zero(n) + 1) / 2);
        std::size_t x1 = (x0 + n / x0) / 2;
        while (x1 < x0) {
            x0 = x1;
            x1 = (x0 + n / x0) / 2;
        }
        return (x0 * x0 == n) ? x0 : (x0 + 1);
    }
}

// Public API
export {

    /**
     * @brief Computes the absolute difference between two arithmetic values: @f$ |a - b| @f$.
     *
     * Safe against unsigned underflow by ensuring subtraction always subtracts
     * the smaller operand from the larger operand.
     *
     * @tparam T Arithmetic type (integral or floating-point).
     * @param a First value.
     * @param b Second value.
     * @return Absolute difference @f$ |a - b| @f$.
     */
    template<typename T>
        requires std::is_arithmetic_v<T>
    [[nodiscard]] constexpr T diff(T a, T b) noexcept
    {
        return (a > b) ? (a - b) : (b - a);
    }

    /**
     * @brief Computes the squared Euclidean distance between two 2D points.
     *
     * Formula: @f$ (x_1 - x_2)^2 + (y_1 - y_2)^2 @f$.
     *
     * Exact and avoids square root computation. Useful for radius comparisons:
     * `DistanceSquared(x1, y1, x2, y2) <= radius * radius`.
     *
     * @param x1 Horizontal coordinate of the first point.
     * @param y1 Vertical coordinate of the first point.
     * @param x2 Horizontal coordinate of the second point.
     * @param y2 Vertical coordinate of the second point.
     * @return Squared Euclidean distance.
     */
    [[nodiscard]] constexpr std::size_t DistanceSquared(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2) noexcept
    {
        const std::size_t diff_x = diff(x1, x2);
        const std::size_t diff_y = diff(y1, y2);

        return diff_x * diff_x + diff_y * diff_y;
    }

    /**
     * @brief Computes the Euclidean distance (rounded up to the next integer) between two 2D points.
     *
     * Formula: @f$ \lceil \sqrt{(x_1 - x_2)^2 + (y_1 - y_2)^2} \rceil @f$.
     *
     * When evaluated during constant evaluation (`std::is_constant_evaluated()`),
     * uses branchless integer Newton-Raphson calculation. At runtime, utilizes
     * hardware floating-point `std::sqrt` and `std::ceil`.
     *
     * @param x1 Horizontal coordinate of the first point.
     * @param y1 Vertical coordinate of the first point.
     * @param x2 Horizontal coordinate of the second point.
     * @param y2 Vertical coordinate of the second point.
     * @return Ceiling of the Euclidean distance.
     */
    [[nodiscard]] constexpr std::size_t Distance(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2) noexcept
    {
        const std::size_t diff_x = diff(x1, x2);
        const std::size_t diff_y = diff(y1, y2);

        const std::size_t dist = diff_x * diff_x + diff_y * diff_y;

        if (std::is_constant_evaluated()) {
            return Detail::CeilISqrt(dist);
        } else {
            return static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(dist))));
        }
    }

    /**
     * @brief Computes the Manhattan (L1 / taxicab / 4-directional) distance between two 2D points.
     *
     * Formula: @f$ |x_1 - x_2| + |y_1 - y_2| @f$.
     *
     * Measures the minimal number of steps required to travel between two points
     * allowing only cardinal (North, South, East, West) movement.
     *
     * @param x1 Horizontal coordinate of the first point.
     * @param y1 Vertical coordinate of the first point.
     * @param x2 Horizontal coordinate of the second point.
     * @param y2 Vertical coordinate of the second point.
     * @return Manhattan distance in grid steps.
     */
    [[nodiscard]] constexpr std::size_t ManhattanDistance(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2) noexcept
    {
        return diff(x1, x2) + diff(y1, y2);
    }

    /**
     * @brief Computes the Chebyshev (L-infinity / chessboard / 8-directional) distance between two 2D points.
     *
     * Formula: @f$ \max(|x_1 - x_2|, |y_1 - y_2|) @f$.
     *
     * Measures the minimal number of king moves required to travel between two points
     * on a chessboard (allowing orthogonal and diagonal movement with uniform step cost).
     *
     * @param x1 Horizontal coordinate of the first point.
     * @param y1 Vertical coordinate of the first point.
     * @param x2 Horizontal coordinate of the second point.
     * @param y2 Vertical coordinate of the second point.
     * @return Chebyshev distance in grid steps.
     */
    [[nodiscard]] constexpr std::size_t ChebyshevDistance(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2) noexcept
    {
        return std::max(diff(x1, x2), diff(y1, y2));
    }

} // end of export

} // end of namespace RL

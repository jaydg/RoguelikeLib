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

    template<typename T>
    [[nodiscard]] constexpr T diff(T a, T b) noexcept
    {
        return (a > b) ? (a - b) : (b - a);
    }

    [[nodiscard]] constexpr std::size_t DistanceSquared(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2) noexcept
    {
        const std::size_t diff_x = diff(x1, x2);
        const std::size_t diff_y = diff(y1, y2);

        return diff_x * diff_x + diff_y * diff_y;
    }

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

} // end of export

} // end of namespace RL

module;

//////////////////////////////////////////////////////////////////////////
// Distance calculation
//////////////////////////////////////////////////////////////////////////

export module rl.distance;

import std;

namespace RL
{

// Public API
export {

    template<typename T>
    T diff(T a, T b)
    {
        return (a > b) ? (a - b) : (b - a);
    }

    [[nodiscard]] inline std::size_t Distance(std::size_t x1, std::size_t y1, std::size_t x2, std::size_t y2) noexcept
    {
        const std::size_t diff_x = diff(x1, x2);
        const std::size_t diff_y = diff(y1, y2);

        const double dist = static_cast<double>(diff_x * diff_x + diff_y * diff_y);

        return static_cast<std::size_t>(std::ceil(std::sqrt(dist)));
    }

} // end of export

} // end of namespace RL

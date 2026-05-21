module;

//////////////////////////////////////////////////////////////////////////
// Distance calculation
//////////////////////////////////////////////////////////////////////////

export module rl.distance;

import std;

namespace RL {

//////////////////////////////////////////////////////////////////////////
// To speed up distance calculation
std::vector<std::size_t> square_root;
//////////////////////////////////////////////////////////////////////////

void InitSquareRoot(std::size_t size)
{
    const std::size_t old_size = square_root.size();

    if(size < old_size) {
        return;
    }

    size++;
    square_root.resize(size);

    // count square root
    for(std::size_t a = old_size; a < size; ++a) {
        square_root[a] = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(a))));
    }
}

// Public API
export {

    template<typename T>
    T diff(T a, T b){
        return (a > b) ? (a - b) : (b - a);
    }

    std::size_t Distance(const std::size_t& x1, const std::size_t& y1, const std::size_t& x2, const std::size_t& y2)
    {
        const std::size_t diff_x = diff(x1, x2);
        const std::size_t diff_y = diff(y1, y2);

        const std::size_t dist = diff_x * diff_x + diff_y * diff_y;

        if(dist >= square_root.size()) {
            InitSquareRoot(dist);
        }

        return square_root[dist];
    }

} // end of export

} // end of namespace RL

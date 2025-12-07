//////////////////////////////////////////////////////////////////////////
// Distance calculation
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef RL_DISTANCE_H
#define RL_DISTANCE_H

#include <cmath>

namespace RL {

template<typename T>
T diff(T a, T b){
    return (a > b) ? (a - b) : (b - a);
}

//////////////////////////////////////////////////////////////////////////
// To speed up distance calculation
static std::vector <size_t> square_root;
//////////////////////////////////////////////////////////////////////////

static
void InitSquareRoot(size_t size)
{
    const size_t old_size = square_root.size();

    if(size < old_size) {
        return;
    }

    size++;
    square_root.resize(size);

    // count square root
    for(size_t a = old_size; a < size; ++a) {
        square_root[a] = static_cast<size_t>(ceil(std::sqrt(static_cast<double>(a))));
    }
}

static
size_t Distance(const size_t& x1, const size_t& y1, const size_t& x2, const size_t& y2)
{
    const size_t diff_x = diff(x1, x2);
    const size_t diff_y = diff(y1, y2);

    const size_t dist = diff_x * diff_x + diff_y * diff_y;

    if(dist >= square_root.size()) {
        InitSquareRoot(dist);
    }

    return square_root[dist];
}

} // end of namespace RL

#endif

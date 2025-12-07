#ifndef POSITION_H
#define POSITION_H

#include <limits>

namespace RL {
struct Position {
    static constexpr size_t invalid = std::numeric_limits<size_t>::max();

    size_t x;
    size_t y;

    Position()
    {
        x = invalid;
        y = invalid;
    }

    Position(size_t a_x, size_t a_y): x(a_x), y(a_y) {};

    const Position& operator+=(const Position& r)
    {
        x += r.x;
        y += r.y;
        return *this;
    }

    const Position& operator-=(const Position& r)
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }

    bool operator==(const Position& r) const
    {
        if(x == r.x && y == r.y) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const Position& r) const
    {
        return !operator==(r);
    }
};

typedef Position Size; // alias name

} // namespace RL

#endif // POSITION_H

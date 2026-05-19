#ifndef POSITION_H
#define POSITION_H

#include <limits>
#include <vector>

#include "distance.h"

namespace RL {
inline int Sign(int n) {
    return (n > 0) ? 1 : ((n == 0) ? 0 : -1);
}

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

    [[nodiscard]] size_t Distance(const Position &other) const
    {
        return RL::Distance(x, y, other.x, other.y);
    }

    [[nodiscard]] std::vector<Position> BuildBresenhamLine(const Position& p2) const
    {
        std::vector<Position> ret;

        int x1 = x, y1 = y;
        int x2 = p2.x, y2 = p2.y;

        int xstep = Sign(x2 - x1);
        int ystep = Sign(y2 - y1);

        int xc = x1;
        int yc = y1;

        ret.emplace_back(xc, yc);

        if (x1 == x2 && y1 == y2) {
            return ret;
        }

        if (std::abs(x2 - x1) >= std::abs(y2 - y1)) {
            int acc = std::abs(x2 - x1);
            do {
                xc += xstep;
                acc += 2 * std::abs(y2 - y1);

                if (acc >= 2 * std::abs(x2 - x1)) {
                    acc -= 2 * std::abs(x2 - x1);
                    yc += ystep;
                }
                ret.emplace_back(xc, yc);
            } while (xc != x2);
        } else {
            int acc = std::abs(y2 - y1);
            do {
                yc += ystep;
                acc += 2 * std::abs(x2 - x1);

                if (acc >= 2 * std::abs(y2 - y1)) {
                    acc -= 2 * std::abs(y2 - y1);
                    xc += xstep;
                }
                ret.emplace_back(xc, yc);
            } while (yc != y2);
        }

        return ret;
    }
};

typedef Position Size; // alias name

} // namespace RL

#endif // POSITION_H

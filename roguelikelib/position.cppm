module;

export module rl.position;

import rl.distance;
import std;

export namespace RL {

inline int Sign(int n) {
    return (n > 0) ? 1 : ((n == 0) ? 0 : -1);
}

struct Position {
    static constexpr std::size_t invalid = std::numeric_limits<std::size_t>::max();

    std::size_t x;
    std::size_t y;

    Position()
    {
        x = invalid;
        y = invalid;
    }

    Position(std::size_t a_x, std::size_t a_y): x(a_x), y(a_y) {};

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
        return (x == r.x && y == r.y);
    }

    bool operator!=(const Position& r) const
    {
        return !(*this == r);
    }

    [[nodiscard]] std::size_t Distance(const Position &other) const
    {
        return RL::Distance(x, y, other.x, other.y);
    }

    [[nodiscard]] std::vector<Position> BuildBresenhamLine(const Position& p2) const
    {
        std::vector<Position> ret;

        int x1 = static_cast<int>(x), y1 = static_cast<int>(y);
        int x2 = static_cast<int>(p2.x), y2 = static_cast<int>(p2.y);

        int xstep = Sign(x2 - x1);
        int ystep = Sign(y2 - y1);

        int xc = x1;
        int yc = y1;

        ret.emplace_back(static_cast<std::size_t>(xc), static_cast<std::size_t>(yc));

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
                ret.emplace_back(static_cast<std::size_t>(xc), static_cast<std::size_t>(yc));
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
                ret.emplace_back(static_cast<std::size_t>(xc), static_cast<std::size_t>(yc));
            } while (yc != y2);
        }

        return ret;
    }
};

using Size = Position;

} // namespace RL

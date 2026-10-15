module;

export module rl.position;

import rl.distance;
import std;

export namespace RL
{

[[nodiscard]] constexpr int Sign(int n) noexcept
{
    return (n > 0) - (n < 0);
}

struct Position {
    static constexpr std::size_t invalid = std::numeric_limits<std::size_t>::max();

    std::size_t x = invalid;
    std::size_t y = invalid;

    constexpr Position() noexcept = default;

    constexpr Position(std::size_t a_x, std::size_t a_y) noexcept : x(a_x), y(a_y) {}

    constexpr const Position& operator+=(const Position& r) noexcept
    {
        x += r.x;
        y += r.y;
        return *this;
    }

    constexpr const Position& operator-=(const Position& r) noexcept
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }

    constexpr bool operator==(const Position& r) const noexcept
    {
        return (x == r.x && y == r.y);
    }

    constexpr bool operator!=(const Position& r) const noexcept
    {
        return !(*this == r);
    }

    [[nodiscard]] std::string toString() const
    {
        return std::format("(x: {}, y: {})", x, y);
    }

    [[nodiscard]] std::size_t Distance(const Position &other) const noexcept
    {
        return RL::Distance(x, y, other.x, other.y);
    }

    [[nodiscard]] constexpr std::size_t DistanceSquared(const Position &other) const noexcept
    {
        return RL::DistanceSquared(x, y, other.x, other.y);
    }
    // Traverses points on the Bresenham line to p2 without allocating memory.
    // If Callback returns bool, returning false stops traversal early.
    template <typename Callback>
    void ForEachBresenham(const Position& p2, Callback&& callback) const
    {
        int x1 = static_cast<int>(x), y1 = static_cast<int>(y);
        int x2 = static_cast<int>(p2.x), y2 = static_cast<int>(p2.y);

        int xstep = Sign(x2 - x1);
        int ystep = Sign(y2 - y1);

        int xc = x1;
        int yc = y1;

        auto step = [&](int xpos, int ypos) -> bool {
            if constexpr (std::is_invocable_r_v<bool, Callback, Position>) {
                return callback(Position(static_cast<std::size_t>(xpos), static_cast<std::size_t>(ypos)));
            } else {
                callback(Position(static_cast<std::size_t>(xpos), static_cast<std::size_t>(ypos)));
                return true;
            }
        };

        if (!step(xc, yc) || (x1 == x2 && y1 == y2)) {
            return;
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

                if (!step(xc, yc)) {
                    return;
                }
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

                if (!step(xc, yc)) {
                    return;
                }
            } while (yc != y2);
        }
    }

    // Appends Bresenham line points to an existing vector, avoiding a new heap allocation
    void BuildBresenhamLine(const Position& p2, std::vector<Position>& out) const
    {
        int x1 = static_cast<int>(x), y1 = static_cast<int>(y);
        int x2 = static_cast<int>(p2.x), y2 = static_cast<int>(p2.y);
        const std::size_t count = static_cast<std::size_t>(std::max(std::abs(x2 - x1), std::abs(y2 - y1))) + 1;
        out.reserve(out.size() + count);

        ForEachBresenham(p2, [&](const Position& p) {
            out.push_back(p);
        });
    }

    [[nodiscard]] std::vector<Position> BuildBresenhamLine(const Position& p2) const
    {
        std::vector<Position> ret;
        BuildBresenhamLine(p2, ret);
        return ret;
    }
};

using Size = Position;

} // namespace RL

template <>
struct std::hash<RL::Position> {
    [[nodiscard]] std::size_t operator()(const RL::Position& p) const noexcept
    {
        std::uint64_t h = p.x * 0x9e3779b97f4a7c15ULL;
        h ^= (p.y + 0x517cc1b727220a95ULL) + (h << 6) + (h >> 2);
        return static_cast<std::size_t>(h);
    }
};

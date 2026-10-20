/**
 * @file position.cppm
 * @brief 2D discrete coordinate and geometric primitive types.
 */

module;

export module rl.position;

import rl.directions;
import rl.distance;
import std;

export namespace RL
{

/**
 * @brief Computes the sign of an integer using branchless arithmetic.
 *
 * @param n Integer value to check.
 * @return 1 if n > 0, -1 if n < 0, or 0 if n == 0.
 */
[[nodiscard]] constexpr int Sign(int n) noexcept
{
    return (n > 0) - (n < 0);
}

/**
 * @brief Represents a discrete 2D point or coordinate on a grid.
 *
 * Coordinates are represented as unsigned 64-bit integers (`std::size_t`),
 * aligning with 2D array and container indexing (e.g., @ref CMap and @ref CMatrix).
 *
 * Provides translation operators, standard distance metrics (Euclidean,
 * Manhattan, Chebyshev), Bresenham line traversal, string representation,
 * and hash support for standard associative containers.
 */
struct Position {
    /**
     * @brief Sentinel value indicating an uninitialized or invalid coordinate.
     */
    static constexpr std::size_t invalid = std::numeric_limits<std::size_t>::max();

    /**
     * @brief Horizontal column coordinate (0-indexed from left).
     */
    std::size_t x = invalid;

    /**
     * @brief Vertical row coordinate (0-indexed from top).
     */
    std::size_t y = invalid;

    /**
     * @brief Default constructor. Initializes both coordinates to @ref invalid.
     */
    constexpr Position() noexcept = default;

    /**
     * @brief Constructs a position with explicit x and y coordinates.
     * @param a_x Horizontal coordinate.
     * @param a_y Vertical coordinate.
     */
    constexpr Position(std::size_t a_x, std::size_t a_y) noexcept : x(a_x), y(a_y) {}

    /**
     * @brief Adds another position component-wise to this position.
     * @param r Position to add.
     * @return Reference to this position after addition.
     */
    constexpr const Position& operator+=(const Position& r) noexcept
    {
        x += r.x;
        y += r.y;
        return *this;
    }

    /**
     * @brief Subtracts another position component-wise from this position.
     * @param r Position to subtract.
     * @return Reference to this position after subtraction.
     */
    constexpr const Position& operator-=(const Position& r) noexcept
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }

    /**
     * @brief Checks if two positions are equal.
     * @param r Position to compare against.
     * @return True if both x and y coordinates are equal.
     */
    constexpr bool operator==(const Position& r) const noexcept
    {
        return (x == r.x && y == r.y);
    }

    /**
     * @brief Checks if two positions are not equal.
     * @param r Position to compare against.
     * @return True if either x or y coordinates differ.
     */
    constexpr bool operator!=(const Position& r) const noexcept
    {
        return !(*this == r);
    }

    /**
     * @brief Formats this position as a human-readable string.
     * @return String in the format `"(x: X, y: Y)"`.
     */
    [[nodiscard]] std::string toString() const
    {
        return std::format("(x: {}, y: {})", x, y);
    }

    /**
     * @brief Computes the Euclidean distance (rounded up to the next integer) to another position.
     *
     * Calculated as @f$ \lceil \sqrt{(x_1 - x_2)^2 + (y_1 - y_2)^2} \rceil @f$.
     *
     * @param other Target position.
     * @return Ceiling of the Euclidean distance.
     */
    [[nodiscard]] constexpr std::size_t Distance(const Position &other) const noexcept
    {
        return RL::Distance(x, y, other.x, other.y);
    }

    /**
     * @brief Computes the squared Euclidean distance to another position.
     *
     * Avoids the square root operation and is exact:
     * @f$ (x_1 - x_2)^2 + (y_1 - y_2)^2 @f$.
     *
     * @param other Target position.
     * @return Squared Euclidean distance.
     */
    [[nodiscard]] constexpr std::size_t DistanceSquared(const Position &other) const noexcept
    {
        return RL::DistanceSquared(x, y, other.x, other.y);
    }

    /**
     * @brief Computes the Manhattan (L1 / taxicab / 4-way movement) distance to another position.
     *
     * Calculated as @f$ |x_1 - x_2| + |y_1 - y_2| @f$.
     *
     * @param other Target position.
     * @return Manhattan distance in grid steps.
     */
    [[nodiscard]] constexpr std::size_t ManhattanDistance(const Position &other) const noexcept
    {
        return RL::ManhattanDistance(x, y, other.x, other.y);
    }

    /**
     * @brief Computes the Chebyshev (L-infinity / chessboard / 8-way movement) distance to another position.
     *
     * Calculated as @f$ \max(|x_1 - x_2|, |y_1 - y_2|) @f$.
     *
     * @param other Target position.
     * @return Chebyshev distance in grid steps.
     */
    [[nodiscard]] constexpr std::size_t ChebyshevDistance(const Position &other) const noexcept
    {
        return RL::ChebyshevDistance(x, y, other.x, other.y);
    }

    /**
     * @brief The direction to step in from this position to an adjacent one.
     *
     * The step's change of x and y is that of @ref DirectionDelta for the direction returned.
     *
     * @param to The position to step onto.
     * @return The direction, or nothing if @p to is this position or not adjacent to it.
     */
    [[nodiscard]] std::optional<EDirection> DirectionTo(const Position &to) const
    {
        const int dx = static_cast<int>(to.x) - static_cast<int>(x);
        const int dy = static_cast<int>(to.y) - static_cast<int>(y);

        for (int direction = EDirectionMin; direction <= EDirectionMax; ++direction) {
            if (DirectionDelta(static_cast<EDirection>(direction)) == std::pair{dx, dy}) {
                return static_cast<EDirection>(direction);
            }
        }

        return std::nullopt;
    }

    /**
     * @brief Traverses grid points on the Bresenham line to @p p2 without dynamic memory allocation.
     *
     * Calls @p callback for each point starting at this position and ending at @p p2.
     * If @p callback returns a `bool`, returning `false` aborts traversal early.
     * If @p callback returns `void`, all points along the line are traversed.
     *
     * @tparam Callback Callable with signature `bool(Position)` or `void(Position)`.
     * @param p2 Destination position.
     * @param callback Function or lambda invoked for each rasterized point.
     */
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

    /**
     * @brief Appends all points on the Bresenham line to @p p2 into an existing vector.
     *
     * Pre-reserves capacity in @p out to avoid reallocations.
     *
     * @param p2 Destination position.
     * @param out Vector to which line points will be appended.
     */
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

    /**
     * @brief Generates and returns a vector containing all points on the Bresenham line to @p p2.
     *
     * @param p2 Destination position.
     * @return Vector of positions starting with `*this` and ending with @p p2.
     */
    [[nodiscard]] std::vector<Position> BuildBresenhamLine(const Position& p2) const
    {
        std::vector<Position> ret;
        BuildBresenhamLine(p2, ret);
        return ret;
    }
};

/**
 * @brief Type alias for 2D rectangular dimensions (width and height).
 *
 * In @ref Size, `x` represents width (`w`) and `y` represents height (`h`).
 */
using Size = Position;

} // namespace RL

/**
 * @brief Hash specialization for @ref RL::Position.
 *
 * Enables storing @ref RL::Position objects in `std::unordered_set` and
 * as keys in `std::unordered_map`. Uses splitmix-style 64-bit bit-mixing
 * to minimize hash collisions.
 */
template <>
struct std::hash<RL::Position> {
    /**
     * @brief Computes a 64-bit hash of an @ref RL::Position.
     * @param p Position to hash.
     * @return Computed hash value.
     */
    [[nodiscard]] std::size_t operator()(const RL::Position& p) const noexcept
    {
        std::uint64_t h = p.x * 0x9e3779b97f4a7c15ULL;
        h ^= (p.y + 0x517cc1b727220a95ULL) + (h << 6) + (h >> 2);
        return static_cast<std::size_t>(h);
    }
};

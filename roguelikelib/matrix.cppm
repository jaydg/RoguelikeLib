//////////////////////////////////////////////////////////////////////////
// Generic matrix class
//////////////////////////////////////////////////////////////////////////

module;

export module rl.matrix;

import rl.position;
import std;

export namespace RL
{

enum class Neighbors {
    All8,       // all 8 surrounding directions
    Cardinal4   // only the 4 cardinal directions (N, S, W, E)
};

class EOutOfBoundException : public std::exception
{
private:
    std::string message;
public:
    EOutOfBoundException(const Position pos, const Size size)
    {
        message = "Given position " + pos.toString() + " is out of bounds " +
                  "for map with dimensions " + size.toString();
    }

    const char* what() const noexcept
    {
        return message.c_str();
    }
};

template <typename T>
class CMatrix
{
public:
    CMatrix() {};

    CMatrix(Size size_, const T& defval)
    {
        size = size_;
        data.resize(size.x * size.y, defval);
    }

    [[nodiscard]] constexpr bool inside(std::size_t x, std::size_t y) const noexcept
    {
        return (x < size.x && y < size.y);
    }

    [[nodiscard]] constexpr bool inside(const Position &pos) const noexcept
    {
        return inside(pos.x, pos.y);
    }

    [[noreturn]] static void ThrowOutOfBounds(std::size_t x, std::size_t y, Size size)
    {
        throw EOutOfBoundException(Position(x, y), size);
    }

    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, size);
        }

        return data[x * size.y + y];
    }

    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y) const
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, size);
        }

        return data[x * size.y + y];
    }

    [[nodiscard]] decltype(auto) get(const Position pos)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, size);
        }

        return data[pos.x * size.y + pos.y];
    }

    [[nodiscard]] decltype(auto) get(const Position pos) const
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, size);
        }

        return data[pos.x * size.y + pos.y];
    }

    void set(std::size_t x, std::size_t y, const T& val)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, size);
        }

        data[x * size.y + y] = val;
    }

    void set(std::size_t x, std::size_t y, T&& val)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, size);
        }

        data[x * size.y + y] = std::move(val);
    }

    void set(const Position pos, const T& val)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, size);
        }

        data[pos.x * size.y + pos.y] = val;
    }

    void set(const Position pos, T&& val)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, size);
        }

        data[pos.x * size.y + pos.y] = std::move(val);
    }

    // Unchecked accessors for hot paths that already validate bounds
    [[nodiscard]] decltype(auto) get_unchecked(std::size_t x, std::size_t y) noexcept
    {
        return data[x * size.y + y];
    }

    [[nodiscard]] decltype(auto) get_unchecked(std::size_t x, std::size_t y) const noexcept
    {
        return data[x * size.y + y];
    }

    [[nodiscard]] decltype(auto) get_unchecked(const Position pos) noexcept
    {
        return data[pos.x * size.y + pos.y];
    }

    [[nodiscard]] decltype(auto) get_unchecked(const Position pos) const noexcept
    {
        return data[pos.x * size.y + pos.y];
    }

    void set_unchecked(std::size_t x, std::size_t y, const T& val) noexcept
    {
        data[x * size.y + y] = val;
    }

    void set_unchecked(std::size_t x, std::size_t y, T&& val) noexcept
    {
        data[x * size.y + y] = std::move(val);
    }

    void set_unchecked(const Position pos, const T& val) noexcept
    {
        data[pos.x * size.y + pos.y] = val;
    }

    void set_unchecked(const Position pos, T&& val) noexcept
    {
        data[pos.x * size.y + pos.y] = std::move(val);
    }

    // 2D subscripting operator() for unchecked element access
    [[nodiscard]] constexpr decltype(auto) operator()(std::size_t x, std::size_t y) noexcept
    {
        return data[x * size.y + y];
    }

    [[nodiscard]] constexpr decltype(auto) operator()(std::size_t x, std::size_t y) const noexcept
    {
        return data[x * size.y + y];
    }

    [[nodiscard]] constexpr decltype(auto) operator()(const Position pos) noexcept
    {
        return data[pos.x * size.y + pos.y];
    }

    [[nodiscard]] constexpr decltype(auto) operator()(const Position pos) const noexcept
    {
        return data[pos.x * size.y + pos.y];
    }

    // Flat iterators for linear sequential traversals
    [[nodiscard]] auto begin() noexcept { return data.begin(); }
    [[nodiscard]] auto end() noexcept { return data.end(); }
    [[nodiscard]] auto begin() const noexcept { return data.begin(); }
    [[nodiscard]] auto end() const noexcept { return data.end(); }
    [[nodiscard]] auto cbegin() const noexcept { return data.cbegin(); }
    [[nodiscard]] auto cend() const noexcept { return data.cend(); }

    // Contiguous span access (for non-bool types)
    [[nodiscard]] auto data_span() noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<T>(data);
    }

    [[nodiscard]] auto data_span() const noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<const T>(data);
    }

    // In column-major layout, each column is contiguous in memory
    [[nodiscard]] auto column(std::size_t x) noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<T>(&data[x * size.y], size.y);
    }

    [[nodiscard]] auto column(std::size_t x) const noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<const T>(&data[x * size.y], size.y);
    }

    [[nodiscard]] Size getSize() const
    {
        return size;
    }

    [[nodiscard]] std::size_t getWidth() const
    {
        return size.x;
    }

    [[nodiscard]] std::size_t getHeight() const
    {
        return size.y;
    }

    // Bulk-set every element to val without per-element bounds checks
    void fill(const T& val)
    {
        std::fill(data.begin(), data.end(), val);
    }

    // Original: compare by equality
    [[nodiscard]] int CountNeighbors(
        const Position& pos, const T& value,
        Neighbors mode = Neighbors::Cardinal4) const
    {
        return CountNeighborsImpl(pos,
        [&value](const T & cell) {
            return cell == value;
        }, mode);
    }

    // compare by predicate (e.g. for CTile a lambda calling getType())
    template <typename Pred>
    requires std::invocable<Pred, const T&>
    && std::convertible_to<std::invoke_result_t<Pred, const T&>, bool>
    [[nodiscard]] int CountNeighbors(
        const Position& pos, Pred&& predicate,
        Neighbors mode = Neighbors::Cardinal4) const
    {
        return CountNeighborsImpl(pos, std::forward<Pred>(predicate), mode);
    }

    bool FloodFill(Position start, T value, bool diagonal = true, int gradient = 0, Position end = Position(-1, -1))
    {
        if (!inside(start)) {
            return false;
        }

        // flood fill room
        T area_value = get_unchecked(start.x, start.y);
        set_unchecked(start.x, start.y, value);

        std::vector<Position> positions;
        positions.reserve(64);
        positions.emplace_back(start);

        std::size_t head = 0;
        bool reached_end = false;

        while (head < positions.size()) {
            const Position current = positions[head++];

            // Fill only to the end?
            if (end.x != Position::invalid && end == current) {
                reached_end = true;
                break;
            }

            std::size_t pos_x = current.x;
            std::size_t pos_y = current.y;

            T this_value = get_unchecked(pos_x, pos_y);

            if (pos_x > 0)
                if (get_unchecked(pos_x - 1, pos_y) == area_value) {
                    set_unchecked(pos_x - 1, pos_y, this_value + gradient);
                    positions.emplace_back(pos_x - 1, pos_y);
                }

            if (pos_x < getWidth() - 1)
                if (get_unchecked(pos_x + 1, pos_y) == area_value) {
                    set_unchecked(pos_x + 1, pos_y, this_value + gradient);
                    positions.emplace_back(pos_x + 1, pos_y);
                }

            if (pos_y > 0)
                if (get_unchecked(pos_x, pos_y - 1) == area_value) {
                    set_unchecked(pos_x, pos_y - 1, this_value + gradient);
                    positions.emplace_back(pos_x, pos_y - 1);
                }

            if (pos_y < getHeight() - 1)
                if (get_unchecked(pos_x, pos_y + 1) == area_value) {
                    set_unchecked(pos_x, pos_y + 1, this_value + gradient);
                    positions.emplace_back(pos_x, pos_y + 1);
                }

            if (diagonal) {
                if (pos_x > 0 && pos_y > 0)
                    if (get_unchecked(pos_x - 1, pos_y - 1) == area_value) {
                        set_unchecked(pos_x - 1, pos_y - 1, this_value + gradient);
                        positions.emplace_back(pos_x - 1, pos_y - 1);
                    }

                if (pos_x < getWidth() - 1 && pos_y < getHeight() - 1)
                    if (get_unchecked(pos_x + 1, pos_y + 1) == area_value) {
                        set_unchecked(pos_x + 1, pos_y + 1, this_value + gradient);
                        positions.emplace_back(pos_x + 1, pos_y + 1);
                    }

                if (pos_x < getWidth() - 1 && pos_y > 0)
                    if (get_unchecked(pos_x + 1, pos_y - 1) == area_value) {
                        set_unchecked(pos_x + 1, pos_y - 1, this_value + gradient);
                        positions.emplace_back(pos_x + 1, pos_y - 1);
                    }

                if (pos_x > 0 && pos_y < getHeight() - 1)
                    if (get_unchecked(pos_x - 1, pos_y + 1) == area_value) {
                        set_unchecked(pos_x - 1, pos_y + 1, this_value + gradient);
                        positions.emplace_back(pos_x - 1, pos_y + 1);
                    }
            }
        }

        if (end.x != Position::invalid) {
            return reached_end;
        }

        return true;
    }

private:
    Size size;
    std::vector<T> data;

    template <typename Pred>
    [[nodiscard]] int CountNeighborsImpl(
        const Position& pos, Pred&& predicate, Neighbors mode) const
    {
        const int sx = static_cast<int>(size.x);
        const int sy = static_cast<int>(size.y);
        const int px = static_cast<int>(pos.x);
        const int py = static_cast<int>(pos.y);

        // Fast-path for interior cells (vast majority of cells in a map)
        if (px > 0 && px + 1 < sx && py > 0 && py + 1 < sy) {
            const std::size_t center = pos.x * size.y + pos.y;
            const std::size_t step_y = size.y;

            if (mode == Neighbors::All8) {
                return (predicate(data[center - step_y - 1]) ? 1 : 0) +
                       (predicate(data[center - 1]) ? 1 : 0) +
                       (predicate(data[center + step_y - 1]) ? 1 : 0) +
                       (predicate(data[center - step_y]) ? 1 : 0) +
                       (predicate(data[center + step_y]) ? 1 : 0) +
                       (predicate(data[center - step_y + 1]) ? 1 : 0) +
                       (predicate(data[center + 1]) ? 1 : 0) +
                       (predicate(data[center + step_y + 1]) ? 1 : 0);
            } else {
                return (predicate(data[center - 1]) ? 1 : 0) +
                       (predicate(data[center - step_y]) ? 1 : 0) +
                       (predicate(data[center + step_y]) ? 1 : 0) +
                       (predicate(data[center + 1]) ? 1 : 0);
            }
        }

        // Boundary cells fallback
        static constexpr std::array<std::pair<int, int>, 8> all8 = {{
                {-1, -1}, {0, -1}, {1, -1},
                {-1,  0},          {1,  0},
                {-1,  1}, {0,  1}, {1,  1}
            }
        };

        static constexpr std::array<std::pair<int, int>, 4> cardinal4 = {{
                {0, -1}, {-1, 0}, {1, 0}, {0, 1}
            }
        };

        const auto& offsets = (mode == Neighbors::All8)
                              ? std::span<const std::pair<int, int>>(all8)
                              : std::span<const std::pair<int, int>>(cardinal4);

        int count = 0;
        for (auto[dx, dy] : offsets) {
            const int nx = px + dx;
            const int ny = py + dy;

            if (nx >= 0 && nx < sx && ny >= 0 && ny < sy) {
                if (predicate(data[nx * size.y + ny])) {
                    ++count;
                }
            }
        }

        return count;
    }
};

} // RL

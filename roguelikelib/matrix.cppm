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
    CMatrix() = default;

    CMatrix(const CMatrix&) = default;
    CMatrix& operator=(const CMatrix&) = default;
    CMatrix(CMatrix&&) noexcept = default;
    CMatrix& operator=(CMatrix&&) noexcept = default;

    CMatrix(Size size_, const T& defval = T())
        : dimensions_(size_), data(size_.x * size_.y, defval)
    {
    }

    CMatrix(std::size_t width, std::size_t height, const T& defval = T())
        : CMatrix(Size(width, height), defval)
    {
    }

    [[nodiscard]] constexpr bool inside(std::size_t x, std::size_t y) const noexcept
    {
        return (x < dimensions_.x && y < dimensions_.y);
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
            ThrowOutOfBounds(x, y, dimensions_);
        }

        return data[x * dimensions_.y + y];
    }

    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y) const
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        return data[x * dimensions_.y + y];
    }

    [[nodiscard]] decltype(auto) get(const Position pos)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        return data[pos.x * dimensions_.y + pos.y];
    }

    [[nodiscard]] decltype(auto) get(const Position pos) const
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        return data[pos.x * dimensions_.y + pos.y];
    }

    void set(std::size_t x, std::size_t y, const T& val)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        data[x * dimensions_.y + y] = val;
    }

    void set(std::size_t x, std::size_t y, T&& val)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        data[x * dimensions_.y + y] = std::move(val);
    }

    void set(const Position pos, const T& val)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        data[pos.x * dimensions_.y + pos.y] = val;
    }

    void set(const Position pos, T&& val)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        data[pos.x * dimensions_.y + pos.y] = std::move(val);
    }

    // Unchecked accessors for hot paths that already validate bounds
    [[nodiscard]] decltype(auto) get_unchecked(std::size_t x, std::size_t y) noexcept
    {
        return data[x * dimensions_.y + y];
    }

    [[nodiscard]] decltype(auto) get_unchecked(std::size_t x, std::size_t y) const noexcept
    {
        return data[x * dimensions_.y + y];
    }

    [[nodiscard]] decltype(auto) get_unchecked(const Position pos) noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    [[nodiscard]] decltype(auto) get_unchecked(const Position pos) const noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    void set_unchecked(std::size_t x, std::size_t y, const T& val) noexcept
    {
        data[x * dimensions_.y + y] = val;
    }

    void set_unchecked(std::size_t x, std::size_t y, T&& val) noexcept
    {
        data[x * dimensions_.y + y] = std::move(val);
    }

    void set_unchecked(const Position pos, const T& val) noexcept
    {
        data[pos.x * dimensions_.y + pos.y] = val;
    }

    void set_unchecked(const Position pos, T&& val) noexcept
    {
        data[pos.x * dimensions_.y + pos.y] = std::move(val);
    }

    // 2D subscripting operator() for unchecked element access
    [[nodiscard]] constexpr decltype(auto) operator()(std::size_t x, std::size_t y) noexcept
    {
        return data[x * dimensions_.y + y];
    }

    [[nodiscard]] constexpr decltype(auto) operator()(std::size_t x, std::size_t y) const noexcept
    {
        return data[x * dimensions_.y + y];
    }

    [[nodiscard]] constexpr decltype(auto) operator()(const Position pos) noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    [[nodiscard]] constexpr decltype(auto) operator()(const Position pos) const noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
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
        return std::span<T>(&data[x * dimensions_.y], dimensions_.y);
    }

    [[nodiscard]] auto column(std::size_t x) const noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<const T>(&data[x * dimensions_.y], dimensions_.y);
    }

    [[nodiscard]] Size getSize() const noexcept
    {
        return dimensions_;
    }

    [[nodiscard]] std::size_t getWidth() const noexcept
    {
        return dimensions_.x;
    }

    [[nodiscard]] std::size_t getHeight() const noexcept
    {
        return dimensions_.y;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return data.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return data.empty();
    }

    // In-place reallocation and reset without constructing temporary CMatrix objects
    void resize(Size new_size, const T& defval = T())
    {
        dimensions_ = new_size;
        data.assign(dimensions_.x * dimensions_.y, defval);
    }

    void resize(std::size_t width, std::size_t height, const T& defval = T())
    {
        resize(Size(width, height), defval);
    }

    void clear() noexcept
    {
        data.clear();
        dimensions_ = Size(0, 0);
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

    bool FloodFill(Position start, const T& value, bool diagonal = true, int gradient = 0, Position end = Position())
    {
        if (!inside(start)) {
            return false;
        }

        const T area_value = get_unchecked(start.x, start.y);
        if (area_value == value && gradient == 0) {
            return (end.x == Position::invalid || end == start);
        }

        set_unchecked(start.x, start.y, value);

        std::vector<Position> positions;
        positions.reserve(64);
        positions.emplace_back(start);

        std::size_t head = 0;
        bool reached_end = false;

        const std::size_t max_x = (dimensions_.x > 0) ? dimensions_.x - 1 : 0;
        const std::size_t max_y = (dimensions_.y > 0) ? dimensions_.y - 1 : 0;
        const std::size_t step_y = dimensions_.y;

        while (head < positions.size()) {
            const Position current = positions[head++];

            // Fill only to the end?
            if (end.x != Position::invalid && end == current) {
                reached_end = true;
                break;
            }

            const std::size_t pos_x = current.x;
            const std::size_t pos_y = current.y;
            const std::size_t center = pos_x * step_y + pos_y;

            const T next_value = (gradient == 0) ? value : static_cast<T>(data[center] + gradient);

            if (pos_x > 0) {
                const std::size_t idx = center - step_y;
                if (data[idx] == area_value) {
                    data[idx] = next_value;
                    positions.emplace_back(pos_x - 1, pos_y);
                }
            }

            if (pos_x < max_x) {
                const std::size_t idx = center + step_y;
                if (data[idx] == area_value) {
                    data[idx] = next_value;
                    positions.emplace_back(pos_x + 1, pos_y);
                }
            }

            if (pos_y > 0) {
                const std::size_t idx = center - 1;
                if (data[idx] == area_value) {
                    data[idx] = next_value;
                    positions.emplace_back(pos_x, pos_y - 1);
                }
            }

            if (pos_y < max_y) {
                const std::size_t idx = center + 1;
                if (data[idx] == area_value) {
                    data[idx] = next_value;
                    positions.emplace_back(pos_x, pos_y + 1);
                }
            }

            if (diagonal) {
                if (pos_x > 0 && pos_y > 0) {
                    const std::size_t idx = center - step_y - 1;
                    if (data[idx] == area_value) {
                        data[idx] = next_value;
                        positions.emplace_back(pos_x - 1, pos_y - 1);
                    }
                }

                if (pos_x < max_x && pos_y < max_y) {
                    const std::size_t idx = center + step_y + 1;
                    if (data[idx] == area_value) {
                        data[idx] = next_value;
                        positions.emplace_back(pos_x + 1, pos_y + 1);
                    }
                }

                if (pos_x < max_x && pos_y > 0) {
                    const std::size_t idx = center + step_y - 1;
                    if (data[idx] == area_value) {
                        data[idx] = next_value;
                        positions.emplace_back(pos_x + 1, pos_y - 1);
                    }
                }

                if (pos_x > 0 && pos_y < max_y) {
                    const std::size_t idx = center - step_y + 1;
                    if (data[idx] == area_value) {
                        data[idx] = next_value;
                        positions.emplace_back(pos_x - 1, pos_y + 1);
                    }
                }
            }
        }

        if (end.x != Position::invalid) {
            return reached_end;
        }

        return true;
    }

private:
    Size dimensions_{0, 0};
    std::vector<T> data;

    template <typename Pred>
    [[nodiscard]] int CountNeighborsImpl(
        const Position& pos, Pred&& predicate, Neighbors mode) const
    {
        const int sx = static_cast<int>(dimensions_.x);
        const int sy = static_cast<int>(dimensions_.y);
        const int px = static_cast<int>(pos.x);
        const int py = static_cast<int>(pos.y);

        // Fast-path for interior cells (vast majority of cells in a map)
        if (px > 0 && px + 1 < sx && py > 0 && py + 1 < sy) {
            const std::size_t center = pos.x * dimensions_.y + pos.y;
            const std::size_t step_y = dimensions_.y;

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
                if (predicate(data[nx * dimensions_.y + ny])) {
                    ++count;
                }
            }
        }

        return count;
    }
};

} // RL

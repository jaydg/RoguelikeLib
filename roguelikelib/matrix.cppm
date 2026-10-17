/**
 * @file matrix.cppm
 * @brief Generic 2D matrix container and 2D grid algorithms.
 */

module;

export module rl.matrix;

import rl.position;
import std;

export namespace RL
{

/**
 * @brief Neighborhood topology mode for adjacency queries.
 */
enum class Neighbors {
    All8,       ///< All 8 surrounding diagonal and orthogonal cells (Moore neighborhood).
    Cardinal4   ///< Only the 4 cardinal orthogonal cells (Von Neumann neighborhood: North, South, West, East).
};

/**
 * @brief Exception thrown when accessing coordinates outside @ref CMatrix boundaries.
 */
class EOutOfBoundException : public std::exception
{
private:
    std::string message;
public:
    /**
     * @brief Constructs an out-of-bounds exception with detailed coordinate context.
     * @param pos Requested coordinate that caused the violation.
     * @param size Dimensions of the matrix.
     */
    EOutOfBoundException(const Position pos, const Size size)
    {
        message = "Given position " + pos.toString() + " is out of bounds " +
                  "for map with dimensions " + size.toString();
    }

    /**
     * @brief Explanatory string describing the out-of-bounds access.
     */
    const char* what() const noexcept override
    {
        return message.c_str();
    }
};

/**
 * @brief Generic 2D grid container storing elements in contiguous memory.
 *
 * @tparam T Type of elements stored in the matrix.
 *
 * Memory is organized in **column-major order** (`index = x * height + y`).
 * Under this layout:
 * - Elements within the same column are contiguous in memory (see @ref column).
 * - Linear traversals via @ref begin() / @ref end() or @ref data_span() sweep through memory column by column.
 * - 2D coordinate indexing is 0-based: `x` spans `[0, width - 1]` and `y` spans `[0, height - 1]`.
 */
template <typename T>
class CMatrix
{
public:
    /**
     * @brief Default constructor creating an empty matrix with dimensions (0, 0).
     */
    CMatrix() = default;

    CMatrix(const CMatrix&) = default;
    CMatrix& operator=(const CMatrix&) = default;
    CMatrix(CMatrix&&) noexcept = default;
    CMatrix& operator=(CMatrix&&) noexcept = default;

    /**
     * @brief Constructs a matrix with given dimensions and initializes elements.
     * @param size_ Width and height of the matrix.
     * @param defval Initial value copied to every cell.
     */
    CMatrix(Size size_, const T& defval = T())
        : dimensions_(size_), data(size_.x * size_.y, defval)
    {
    }

    /**
     * @brief Constructs a matrix with explicit width and height.
     * @param width Number of columns.
     * @param height Number of rows.
     * @param defval Initial value copied to every cell.
     */
    CMatrix(std::size_t width, std::size_t height, const T& defval = T())
        : CMatrix(Size(width, height), defval)
    {
    }

    /**
     * @brief Checks if a coordinate pair falls within matrix bounds.
     * @param x Horizontal column index.
     * @param y Vertical row index.
     * @return True if `x < width` and `y < height`.
     */
    [[nodiscard]] constexpr bool inside(std::size_t x, std::size_t y) const noexcept
    {
        return (x < dimensions_.x && y < dimensions_.y);
    }

    /**
     * @brief Checks if a position falls within matrix bounds.
     * @param pos Position to test.
     * @return True if `pos.x < width` and `pos.y < height`.
     */
    [[nodiscard]] constexpr bool inside(const Position &pos) const noexcept
    {
        return inside(pos.x, pos.y);
    }

    /**
     * @brief Helper that throws an @ref EOutOfBoundException.
     * @param x Horizontal column coordinate.
     * @param y Vertical row coordinate.
     * @param size Current matrix dimensions.
     */
    [[noreturn]] static void ThrowOutOfBounds(std::size_t x, std::size_t y, Size size)
    {
        throw EOutOfBoundException(Position(x, y), size);
    }

    /**
     * @brief Accesses an element with bounds checking (mutable).
     * @param x Column index.
     * @param y Row index.
     * @return Reference to the element at `(x, y)`.
     * @throws EOutOfBoundException If `(x, y)` is outside the matrix.
     */
    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        return data[x * dimensions_.y + y];
    }

    /**
     * @brief Accesses an element with bounds checking (const).
     * @param x Column index.
     * @param y Row index.
     * @return Const reference to the element at `(x, y)`.
     * @throws EOutOfBoundException If `(x, y)` is outside the matrix.
     */
    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y) const
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        return data[x * dimensions_.y + y];
    }

    /**
     * @brief Accesses an element at a position with bounds checking (mutable).
     * @param pos Target position.
     * @return Reference to the element at `pos`.
     * @throws EOutOfBoundException If `pos` is outside the matrix.
     */
    [[nodiscard]] decltype(auto) get(const Position pos)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        return data[pos.x * dimensions_.y + pos.y];
    }

    /**
     * @brief Accesses an element at a position with bounds checking (const).
     * @param pos Target position.
     * @return Const reference to the element at `pos`.
     * @throws EOutOfBoundException If `pos` is outside the matrix.
     */
    [[nodiscard]] decltype(auto) get(const Position pos) const
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        return data[pos.x * dimensions_.y + pos.y];
    }

    /**
     * @brief Assigns a value to a cell with bounds checking (copy).
     * @param x Column index.
     * @param y Row index.
     * @param val Value to copy into the cell.
     * @throws EOutOfBoundException If `(x, y)` is outside the matrix.
     */
    void set(std::size_t x, std::size_t y, const T& val)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        data[x * dimensions_.y + y] = val;
    }

    /**
     * @brief Assigns a value to a cell with bounds checking (move).
     * @param x Column index.
     * @param y Row index.
     * @param val Value to move into the cell.
     * @throws EOutOfBoundException If `(x, y)` is outside the matrix.
     */
    void set(std::size_t x, std::size_t y, T&& val)
    {
        if (!inside(x, y)) {
            ThrowOutOfBounds(x, y, dimensions_);
        }

        data[x * dimensions_.y + y] = std::move(val);
    }

    /**
     * @brief Assigns a value to a cell at a position with bounds checking (copy).
     * @param pos Target position.
     * @param val Value to copy into the cell.
     * @throws EOutOfBoundException If `pos` is outside the matrix.
     */
    void set(const Position pos, const T& val)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        data[pos.x * dimensions_.y + pos.y] = val;
    }

    /**
     * @brief Assigns a value to a cell at a position with bounds checking (move).
     * @param pos Target position.
     * @param val Value to move into the cell.
     * @throws EOutOfBoundException If `pos` is outside the matrix.
     */
    void set(const Position pos, T&& val)
    {
        if (!inside(pos)) {
            ThrowOutOfBounds(pos.x, pos.y, dimensions_);
        }

        data[pos.x * dimensions_.y + pos.y] = std::move(val);
    }

    // Unchecked accessors for hot paths that already validate bounds

    /**
     * @brief Unchecked element access by coordinate indices (mutable).
     * @param x Column index.
     * @param y Row index.
     * @return Reference to the element.
     */
    [[nodiscard]] decltype(auto) get_unchecked(std::size_t x, std::size_t y) noexcept
    {
        return data[x * dimensions_.y + y];
    }

    /**
     * @brief Unchecked element access by coordinate indices (const).
     * @param x Column index.
     * @param y Row index.
     * @return Const reference to the element.
     */
    [[nodiscard]] decltype(auto) get_unchecked(std::size_t x, std::size_t y) const noexcept
    {
        return data[x * dimensions_.y + y];
    }

    /**
     * @brief Unchecked element access by position (mutable).
     * @param pos Target position.
     * @return Reference to the element.
     */
    [[nodiscard]] decltype(auto) get_unchecked(const Position pos) noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    /**
     * @brief Unchecked element access by position (const).
     * @param pos Target position.
     * @return Const reference to the element.
     */
    [[nodiscard]] decltype(auto) get_unchecked(const Position pos) const noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    /**
     * @brief Unchecked element assignment by coordinate indices (copy).
     * @param x Column index.
     * @param y Row index.
     * @param val Value to copy.
     */
    void set_unchecked(std::size_t x, std::size_t y, const T& val) noexcept
    {
        data[x * dimensions_.y + y] = val;
    }

    /**
     * @brief Unchecked element assignment by coordinate indices (move).
     * @param x Column index.
     * @param y Row index.
     * @param val Value to move.
     */
    void set_unchecked(std::size_t x, std::size_t y, T&& val) noexcept
    {
        data[x * dimensions_.y + y] = std::move(val);
    }

    /**
     * @brief Unchecked element assignment by position (copy).
     * @param pos Target position.
     * @param val Value to copy.
     */
    void set_unchecked(const Position pos, const T& val) noexcept
    {
        data[pos.x * dimensions_.y + pos.y] = val;
    }

    /**
     * @brief Unchecked element assignment by position (move).
     * @param pos Target position.
     * @param val Value to move.
     */
    void set_unchecked(const Position pos, T&& val) noexcept
    {
        data[pos.x * dimensions_.y + pos.y] = std::move(val);
    }

    // 2D subscripting operator() for unchecked element access

    /**
     * @brief Unchecked 2D call operator for element access by coordinates (mutable).
     * @param x Column index.
     * @param y Row index.
     * @return Reference to the element.
     */
    [[nodiscard]] constexpr decltype(auto) operator()(std::size_t x, std::size_t y) noexcept
    {
        return data[x * dimensions_.y + y];
    }

    /**
     * @brief Unchecked 2D call operator for element access by coordinates (const).
     * @param x Column index.
     * @param y Row index.
     * @return Const reference to the element.
     */
    [[nodiscard]] constexpr decltype(auto) operator()(std::size_t x, std::size_t y) const noexcept
    {
        return data[x * dimensions_.y + y];
    }

    /**
     * @brief Unchecked call operator for element access by position (mutable).
     * @param pos Target position.
     * @return Reference to the element.
     */
    [[nodiscard]] constexpr decltype(auto) operator()(const Position pos) noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    /**
     * @brief Unchecked call operator for element access by position (const).
     * @param pos Target position.
     * @return Const reference to the element.
     */
    [[nodiscard]] constexpr decltype(auto) operator()(const Position pos) const noexcept
    {
        return data[pos.x * dimensions_.y + pos.y];
    }

    // Flat iterators for linear sequential traversals

    /** @brief Returns an iterator to the beginning of the linear buffer. */
    [[nodiscard]] auto begin() noexcept { return data.begin(); }
    /** @brief Returns an iterator to the end of the linear buffer. */
    [[nodiscard]] auto end() noexcept { return data.end(); }
    /** @brief Returns a const iterator to the beginning of the linear buffer. */
    [[nodiscard]] auto begin() const noexcept { return data.begin(); }
    /** @brief Returns a const iterator to the end of the linear buffer. */
    [[nodiscard]] auto end() const noexcept { return data.end(); }
    /** @brief Returns a const iterator to the beginning of the linear buffer. */
    [[nodiscard]] auto cbegin() const noexcept { return data.cbegin(); }
    /** @brief Returns a const iterator to the end of the linear buffer. */
    [[nodiscard]] auto cend() const noexcept { return data.cend(); }

    // Contiguous span access (for non-bool types)

    /**
     * @brief Returns a contiguous `std::span` over the entire underlying buffer (mutable).
     * @note Only available for non-bool element types (`std::vector<bool>` is bit-packed).
     */
    [[nodiscard]] auto data_span() noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<T>(data);
    }

    /**
     * @brief Returns a contiguous `std::span` over the entire underlying buffer (const).
     * @note Only available for non-bool element types (`std::vector<bool>` is bit-packed).
     */
    [[nodiscard]] auto data_span() const noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<const T>(data);
    }

    // In column-major layout, each column is contiguous in memory

    /**
     * @brief Returns a contiguous `std::span` over column `x` (mutable).
     *
     * In column-major storage, elements with the same `x` and varying `y` are contiguous in memory.
     *
     * @param x Column index.
     * @return Span covering all cells `(x, 0)` through `(x, height - 1)`.
     */
    [[nodiscard]] auto column(std::size_t x) noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<T>(&data[x * dimensions_.y], dimensions_.y);
    }

    /**
     * @brief Returns a contiguous `std::span` over column `x` (const).
     *
     * @param x Column index.
     * @return Const span covering all cells `(x, 0)` through `(x, height - 1)`.
     */
    [[nodiscard]] auto column(std::size_t x) const noexcept requires (!std::is_same_v<T, bool>)
    {
        return std::span<const T>(&data[x * dimensions_.y], dimensions_.y);
    }

    /**
     * @brief Returns the 2D dimensions of the matrix.
     * @return @ref Size with width in `x` and height in `y`.
     */
    [[nodiscard]] Size getSize() const noexcept
    {
        return dimensions_;
    }

    /**
     * @brief Returns the number of columns (width) in the matrix.
     */
    [[nodiscard]] std::size_t getWidth() const noexcept
    {
        return dimensions_.x;
    }

    /**
     * @brief Returns the number of rows (height) in the matrix.
     */
    [[nodiscard]] std::size_t getHeight() const noexcept
    {
        return dimensions_.y;
    }

    /**
     * @brief Returns total number of cells (`width * height`).
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
        return data.size();
    }

    /**
     * @brief Checks if the matrix contains zero elements.
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return data.empty();
    }

    // In-place reallocation and reset without constructing temporary CMatrix objects

    /**
     * @brief Resizes the matrix in place, reassigning all elements to `defval`.
     * @param new_size New width and height.
     * @param defval Value to fill the resized matrix with.
     */
    void resize(Size new_size, const T& defval = T())
    {
        dimensions_ = new_size;
        data.assign(dimensions_.x * dimensions_.y, defval);
    }

    /**
     * @brief Resizes the matrix in place with explicit dimensions.
     * @param width New column count.
     * @param height New row count.
     * @param defval Value to fill the resized matrix with.
     */
    void resize(std::size_t width, std::size_t height, const T& defval = T())
    {
        resize(Size(width, height), defval);
    }

    /**
     * @brief Clears the matrix, releasing element memory and resetting dimensions to (0, 0).
     */
    void clear() noexcept
    {
        data.clear();
        dimensions_ = Size(0, 0);
    }

    /**
     * @brief Fills the entire matrix with a given value without per-element bounds checks.
     * @param val Value to assign to every cell.
     */
    void fill(const T& val)
    {
        std::fill(data.begin(), data.end(), val);
    }

    /**
     * @brief Counts adjacent cells whose values equal @p value.
     *
     * @param pos Center cell whose neighbors are inspected.
     * @param value Value compared against neighbor cells using `operator==`.
     * @param mode Neighborhood mode: @ref Neighbors::Cardinal4 or @ref Neighbors::All8.
     * @return Number of matching neighboring cells.
     */
    [[nodiscard]] int CountNeighbors(
        const Position& pos, const T& value,
        Neighbors mode = Neighbors::Cardinal4) const
    {
        return CountNeighborsImpl(pos,
        [&value](const T & cell) {
            return cell == value;
        }, mode);
    }

    /**
     * @brief Counts adjacent cells satisfying a custom unary predicate.
     *
     * @tparam Pred Callable taking `const T&` and returning a type convertible to `bool`.
     * @param pos Center cell whose neighbors are inspected.
     * @param predicate Predicate invoked on each neighbor cell.
     * @param mode Neighborhood mode: @ref Neighbors::Cardinal4 or @ref Neighbors::All8.
     * @return Number of matching neighboring cells.
     */
    template <typename Pred>
    requires std::invocable<Pred, const T&>
    && std::convertible_to<std::invoke_result_t<Pred, const T&>, bool>
    [[nodiscard]] int CountNeighbors(
        const Position& pos, Pred&& predicate,
        Neighbors mode = Neighbors::Cardinal4) const
    {
        return CountNeighborsImpl(pos, std::forward<Pred>(predicate), mode);
    }

    /**
     * @brief Performs a breadth-first flood fill starting at @p start.
     *
     * Replaces contiguous cells matching `get(start)` with @p value.
     * Supports optional diagonal traversal and gradient values for distance/heat maps.
     *
     * @param start Origin cell of the flood fill.
     * @param value Value assigned to flooded cells.
     * @param diagonal When true, floods across all 8 directions; otherwise cardinal 4 directions.
     * @param gradient Value added per expansion step (e.g., +1 for distance transforms). 0 for uniform fill.
     * @param end Optional destination position. If set, stops as soon as @p end is reached.
     * @return True if fill succeeded (or if @p end was reached when specified).
     */
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

} // namespace RL

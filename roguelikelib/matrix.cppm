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

    CMatrix(Size size_, T defval)
    {
        size = size_;
        data.resize(size.x * size.y, defval);
    }

    [[nodiscard]] bool inside(const std::size_t& x, const std::size_t& y) const
    {
        return (x < size.x && y < size.y);
    }

    [[nodiscard]] bool inside(const Position &pos) const
    {
        return inside(pos.x, pos.y);
    }

    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y)
    {
        if (!inside(x, y)) {
            throw EOutOfBoundException(Position(x, y), size);
        }

        return data[x * size.y + y];
    }

    [[nodiscard]] decltype(auto) get(std::size_t x, std::size_t y) const
    {
        if (!inside(x, y)) {
            throw EOutOfBoundException(Position(x, y), size);
        }

        return data[x * size.y + y];
    }

    [[nodiscard]] decltype(auto) get(const Position pos)
    {
        if (!inside(pos)) {
            throw EOutOfBoundException(pos, size);
        }

        return data[pos.x * size.y + pos.y];
    }

    [[nodiscard]] decltype(auto) get(const Position pos) const
    {
        if (!inside(pos)) {
            throw EOutOfBoundException(pos, size);
        }

        return data[pos.x * size.y + pos.y];
    }

    void set(std::size_t x, std::size_t y, T val)
    {
        if (!inside(x, y)) {
            throw EOutOfBoundException(Position(x, y), size);
        }

        data[x * size.y + y] = val;
    }

    void set(const Position pos, T val)
    {
        if (!inside(pos)) {
            throw EOutOfBoundException(pos, size);
        }

        data[pos.x * size.y + pos.y] = val;
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

    // Original: compare by equality
    [[nodiscard]] int CountNeighbors(
        const Position& pos, const T& value,
        Neighbors mode = Neighbors::Cardinal4)
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
        Neighbors mode = Neighbors::Cardinal4)
    {
        return CountNeighborsImpl(pos, std::forward<Pred>(predicate), mode);
    }

    bool FloodFill(Position start, T value, bool diagonal = true, int gradient = 0, Position end = Position(-1, -1))
    {
        // flood fill room
        T area_value = get(start.x, start.y);
        set(start.x, start.y, value);

        std::list<Position> positions;
        positions.emplace_back(start);

        auto m = positions.begin();

        while (m != positions.end()) {

            // Fill only to the end?
            if (end.x != Position::invalid && end == (*m)) {
                break;
            }

            std::size_t pos_x = m->x;
            std::size_t pos_y = m->y;

            T this_value = get(pos_x, pos_y);

            if (pos_x > 0)
                if (get(pos_x - 1, pos_y) == area_value) {
                    set(pos_x - 1, pos_y, this_value + gradient);
                    positions.emplace_back(pos_x - 1, pos_y);
                }

            if (pos_x < getWidth() - 1)
                if (get(pos_x + 1, pos_y) == area_value) {
                    set(pos_x + 1, pos_y, this_value + gradient);
                    positions.emplace_back(pos_x + 1, pos_y);
                }

            if (pos_y > 0)
                if (get(pos_x, pos_y - 1) == area_value) {
                    set(pos_x, pos_y - 1, this_value + gradient);
                    positions.emplace_back(pos_x, pos_y - 1);
                }

            if (pos_y < getHeight() - 1)
                if (get(pos_x, pos_y + 1) == area_value) {
                    set(pos_x, pos_y + 1, this_value + gradient);
                    positions.emplace_back(pos_x, pos_y + 1);
                }

            if (diagonal) {
                if (pos_x > 0 && pos_y > 0)
                    if (get(pos_x - 1, pos_y - 1) == area_value) {
                        set(pos_x - 1, pos_y - 1, this_value + gradient);
                        positions.emplace_back(pos_x - 1, pos_y - 1);
                    }

                if (pos_x < getWidth() - 1 && pos_y < getHeight() - 1)
                    if (get(pos_x + 1, pos_y + 1) == area_value) {
                        set(pos_x + 1, pos_y + 1, this_value + gradient);
                        positions.emplace_back(pos_x + 1, pos_y + 1);
                    }

                if (pos_x < getWidth() - 1 && pos_y > 0)
                    if (get(pos_x + 1, pos_y - 1) == area_value) {
                        set(pos_x + 1, pos_y - 1, this_value + gradient);
                        positions.emplace_back(pos_x + 1, pos_y - 1);
                    }

                if (pos_x > 0 && pos_y < getHeight() - 1)
                    if (get(pos_x - 1, pos_y + 1) == area_value) {
                        set(pos_x - 1, pos_y + 1, this_value + gradient);
                        positions.emplace_back(pos_x - 1, pos_y + 1);
                    }
            }

            m = positions.erase(m);
        }

        // filling to the end cell without success
        if (m == positions.end()) {
            return false;
        }

        if (end.x != Position::invalid && end != (*m)) {
            return false;
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
        // *INDENT-OFF* (protect againt astyle)
        static constexpr std::array<std::pair<int, int>, 8> all8 = {{
                {-1, -1}, {0, -1}, {1, -1},
                {-1,  0},          {1,  0},
                {-1,  1}, {0,  1}, {1,  1}
            }
        };
        // *INDENT-ON*

        static constexpr std::array<std::pair<int, int>, 4> cardinal4 = {{
                {0, -1}, {-1, 0}, {1, 0}, {0, 1}
            }
        };

        const auto& offsets = (mode == Neighbors::All8)
                              ? std::span<const std::pair<int, int>>(all8)
                              : std::span<const std::pair<int, int>>(cardinal4);

        const int px = static_cast<int>(pos.x);
        const int py = static_cast<int>(pos.y);
        int count = 0;

        for (auto[dx, dy] : offsets) {
            const int nx = px + dx;
            const int ny = py + dy;

            if (nx >= 0 && nx < static_cast<int>(size.x) && ny >= 0 && ny < static_cast<int>(size.y))
                if (predicate(data[nx * size.y + ny])) {
                    ++count;
                }
        }

        return count;
    }
};

} // RL

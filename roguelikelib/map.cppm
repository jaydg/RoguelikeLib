//////////////////////////////////////////////////////////////////////////
// Map
//////////////////////////////////////////////////////////////////////////

module;

export module rl.map;

import rl.position;
import rl.tile;
import std;

export namespace RL {

class EOutOfBoundException : public std::exception {
private:
    std::string message;
public:
    EOutOfBoundException(const Position pos, const Size size) {
        message = "Given position " + pos.toString() + " is out of bounds " +
            "for map with dimensions " + size.toString();
    }

    const char* what() const noexcept {
        return message.c_str();
    }
};

class CMap {
private:

    std::vector <CTile> m_map;
    Size size;

public:

    void Resize(std::size_t width, std::size_t height)
    {
        if(width == 0 || height == 0) {
            return;
        }

        size.x = width;
        size.y = height;

        m_map.resize(size.x * size.y);
    }

    void Resize(Size new_size)
    {
        return Resize(new_size.x, new_size.y);
    }

    [[nodiscard]] Size getSize() const
    {
        return size;
    }

    [[nodiscard]] std::size_t GetWidth() const
    {
        return size.x;
    }

    [[nodiscard]] std::size_t GetHeight() const
    {
        return size.y;
    }

    void Clear(std::string_view typ = "wall")
    {
        if (m_map.empty()) {
            return;
        }

        for (std::size_t x = 0; x < size.x; ++x) {
            for (std::size_t y = 0; y < size.y; ++y) {
                m_map[x * size.y + y] = CTile(typ);
            }
        }
    }

    [[nodiscard]] bool OnMap(const std::size_t& x, const std::size_t& y) const
    {
        return (x < size.x && y < size.y);
    }

    [[nodiscard]] bool OnMap(const Position &pos) const
    {
        return OnMap(pos.x, pos.y);
    }

    void SetCell(const std::size_t& x, const std::size_t& y, std::string_view element)
    {
        if (!OnMap(x, y)) {
            throw EOutOfBoundException(Position(x, y), size);
        }

        m_map[x * size.y + y] = CTile(element);
    }

    void SetCell(const Position &pos, std::string_view element)
    {
        return SetCell(pos.x, pos.y, element);
    }

    // r/w access (returns a modifiable reference)
    [[nodiscard]] CTile& GetCell(const std::size_t& x, const std::size_t& y) {
        if (!OnMap(x, y)) {
            throw EOutOfBoundException(Position(x, y), size);
        }

        return m_map[x * size.y + y];
    }

    [[nodiscard]] CTile& GetCell(const Position &pos) {
        return GetCell(pos.x, pos.y);
    }

    // r/o (returns a constant reference to avoid copies)
    [[nodiscard]] const CTile& GetCell(const std::size_t& x, const std::size_t& y) const {
        if (!OnMap(x, y)) {
            throw EOutOfBoundException(Position(x, y), size);
        }

        return m_map[x * size.y + y];
    }

    [[nodiscard]] const CTile& GetCell(const Position &pos) const {
        return GetCell(pos.x, pos.y);
    }
    void PrintMap() const
    {
        for (std::size_t y = 0; y < size.y; ++y) {
            for (std::size_t x = 0; x < size.x; ++x) {
                std::cout << GetCell(x, y).getGlyph();
            }

            std::cout << std::endl;
        }
    }
};

struct SRoom {
    Position corner1, corner2;
    int type{};

    [[nodiscard]] bool IsInRoom(const Position &pos) const
    {
        return (pos.x >= corner1.x && pos.x <= corner2.x && pos.y >= corner1.y && pos.y <= corner2.y);
    }

    [[nodiscard]] bool IsInRoom(const std::size_t x, const std::size_t y) const
    {
        return (x >= corner1.x && x <= corner2.x && y >= corner1.y && y <= corner2.y);
    }
};

} // end of namespace RL

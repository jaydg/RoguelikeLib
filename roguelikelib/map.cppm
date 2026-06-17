//////////////////////////////////////////////////////////////////////////
// Map
//////////////////////////////////////////////////////////////////////////

module;

export module rl.map;

import rl.matrix;
import rl.position;
import rl.tile;
import std;
import stc;

export namespace RL {

class CMap : public CMatrix<CTile> {

public:
    CMap(Size size) : CMatrix<CTile>(size, CTile("wall")) {}

    void Clear(std::string_view typ = "wall")
    {
        for (std::size_t y = 0; y < getHeight(); ++y) {
            for (std::size_t x = 0; x < getWidth(); ++x) {
                get(x, y).setType(typ);
            }
        }
    }

    void SetCell(const std::size_t& x, const std::size_t& y, std::string_view element)
    {
        if (!inside(x, y)) {
            throw EOutOfBoundException(Position(x, y), getSize());
        }

        get(x, y).setType(element);
    }

    void SetCell(const Position &pos, std::string_view element)
    {
        return SetCell(pos.x, pos.y, element);
    }

    void PrintMap() const
    {
        for (std::size_t y = 0; y < getHeight(); ++y) {
            for (std::size_t x = 0; x < getWidth(); ++x) {
                auto tile = get(x, y);
                std::cout << stc::rgb_fg(tile.getColor()) << tile.getGlyph();
            }

            std::cout << stc::reset << std::endl;
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

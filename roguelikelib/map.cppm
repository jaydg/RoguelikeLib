//////////////////////////////////////////////////////////////////////////
// Map
//////////////////////////////////////////////////////////////////////////

module;

export module rl.map;

import rl.matrix;
import rl.position;
import rl.tile;
import std;

export namespace RL
{

class CMap : public CMatrix<CTile>
{

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

    // Copy a map with its top-left corner at origin, clipping at our edges.
    // Listed tile types are transparent to the stamp. Copy tiles directly
    // to retain their colours and properties without consuming randomness.
    void Stamp(const CMap& source, Position origin, std::span<const std::string_view> skip_types = {})
    {
        if (!inside(origin) || source.getWidth() == 0 || source.getHeight() == 0) {
            return;
        }
        // Overlapping self-stamps read the original cells, never earlier writes.
        if (&source == this) {
            const CMap snapshot(source);
            Stamp(snapshot, origin, skip_types);
            return;
        }
        const auto width = std::min(source.getWidth(), getWidth() - origin.x);
        const auto height = std::min(source.getHeight(), getHeight() - origin.y);
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                const auto& tile = source.get(x, y);
                if (std::ranges::find(skip_types, tile.getType()) == skip_types.end()) {
                    get(origin.x + x, origin.y + y) = tile;
                }
            }
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

// A room walled in on all sides, with a way in: the corners are those of
// the walls, and the door is one of the wall's cells
struct SBuilding : SRoom {
    Position door;
};

} // end of namespace RL

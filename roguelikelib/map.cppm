/**
 * @file map.cppm
 * @brief 2D tile map container, stamping operations, and room/building geometric primitives.
 */

module;

//////////////////////////////////////////////////////////////////////////
// Map
//////////////////////////////////////////////////////////////////////////

export module rl.map;

import rl.matrix;
import rl.position;
import rl.tile;
import std;

export namespace RL
{

/**
 * @brief Represents a 2D tile-based roguelike map.
 *
 * Inherits from @ref CMatrix<CTile>, storing @ref CTile elements in column-major order.
 * Provides map clearing, cell modification by element name, and map-in-map
 * stamping operations with transparency and boundary clipping.
 */
class CMap : public CMatrix<CTile>
{

public:
    /**
     * @brief Constructs a map with specified dimensions, initialized with `"wall"` tiles.
     * @param size Width and height of the map.
     */
    CMap(Size size) : CMatrix<CTile>(size, CTile("wall")) {}

    /**
     * @brief Resets all cells in the map to a given tile type name.
     * @param typ Tile type string to apply to every cell (defaults to `"wall"`).
     */
    void Clear(std::string_view typ = "wall")
    {
        for (std::size_t y = 0; y < getHeight(); ++y) {
            for (std::size_t x = 0; x < getWidth(); ++x) {
                get(x, y).setType(typ);
            }
        }
    }

    /**
     * @brief Sets the tile type of a cell at given coordinates with bounds checking.
     * @param x Horizontal column index.
     * @param y Vertical row index.
     * @param element Tile type name to assign (e.g., `"floor"`, `"wall"`, `"door"`).
     * @throws EOutOfBoundException If `(x, y)` is outside the map boundaries.
     */
    void SetCell(const std::size_t& x, const std::size_t& y, std::string_view element)
    {
        if (!inside(x, y)) {
            throw EOutOfBoundException(Position(x, y), getSize());
        }

        get(x, y).setType(element);
    }

    /**
     * @brief Sets the tile type of a cell at a position with bounds checking.
     * @param pos Coordinate position to modify.
     * @param element Tile type name to assign.
     * @throws EOutOfBoundException If @p pos is outside the map boundaries.
     */
    void SetCell(const Position &pos, std::string_view element)
    {
        return SetCell(pos.x, pos.y, element);
    }

    /**
     * @brief Blits and stamps a source map into this map at a given origin offset.
     *
     * Copies whole tiles directly from @p source to preserve glyphs, colors,
     * passability flags, and transparency attributes.
     *
     * - **Boundary Clipping**: Automatically clips to this map's right and bottom edges.
     * - **Tile Filtering**: Cells in @p source whose `getType()` matches any entry in
     *   @p skip_types are left untouched in the destination map.
     * - **Self-Stamp Safety**: Overlapping self-stamps take an internal snapshot to avoid
     *   feedback artifacts.
     *
     * @param source The source map to copy from.
     * @param origin Top-left destination coordinate where @p source will be placed.
     * @param skip_types Optional list of tile type names that act as transparent masks.
     */
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

/**
 * @brief Represents an axis-aligned rectangular room on a map.
 */
struct SRoom {
    Position corner1; ///< Top-left corner coordinate of the room (inclusive).
    Position corner2; ///< Bottom-right corner coordinate of the room (inclusive).
    int type{};       ///< Optional room archetype or theme category identifier.

    /**
     * @brief Checks if a position lies within the room's bounding rectangle (inclusive).
     * @param pos Coordinate to test.
     * @return True if @p pos is inside or on the room boundary.
     */
    [[nodiscard]] bool IsInRoom(const Position &pos) const
    {
        return (pos.x >= corner1.x && pos.x <= corner2.x && pos.y >= corner1.y && pos.y <= corner2.y);
    }

    /**
     * @brief Checks if a coordinate pair lies within the room's bounding rectangle (inclusive).
     * @param x Column index.
     * @param y Row index.
     * @return True if `(x, y)` is inside or on the room boundary.
     */
    [[nodiscard]] bool IsInRoom(const std::size_t x, const std::size_t y) const
    {
        return (x >= corner1.x && x <= corner2.x && y >= corner1.y && y <= corner2.y);
    }
};

/**
 * @brief Represents a walled building with an entrance door.
 *
 * Extends @ref SRoom, where `corner1` and `corner2` delineate the outer wall
 * perimeter (inclusive), and `door` denotes the street-facing door location.
 */
struct SBuilding : SRoom {
    Position door; ///< Coordinate of the doorway tile along the exterior perimeter wall.
};

} // end of namespace RL

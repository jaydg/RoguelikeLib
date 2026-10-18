/**
 * @file tile.cppm
 * @brief Tile definitions, tile templates, global tile registry, and map cell instances.
 */

module;

import rl.randomness;
import std;

export module rl.tile;

namespace RL
{

/**
 * @brief Template definition of a map tile's physical and visual properties.
 *
 * Defines the canonical glyph, base RGB color, light transparency, and movement
 * passability used when instantiating @ref CTile cells on a map.
 */
export struct STileData {
    char32_t glyph = U' ';              ///< Unicode character code point representing the tile glyph.
    std::uint32_t rgb_color = 0xFFFFFF; ///< Base color packed as `0x00RRGGBB`.
    bool transparent = false;           ///< Line-of-sight transparency (true allows FOV light rays through).
    bool passable = false;              ///< Movement passability (true allows characters to traverse the tile).
};

export {

    /**
     * @brief Global registry and factory for tile definitions.
     *
     * Manages built-in tile archetypes (e.g., `"wall"`, `"corridor"`, `"grass"`, `"water"`)
     * and supports registering custom tile types dynamically at runtime.
     *
     * All string keys registered in @ref CTileData have stable lifetime, guaranteeing
     * that @ref CTile::getType() string views remain valid throughout program execution.
     */
    class CTileData {
    private:
        // Lets the registry be searched with a string_view, without building
        // a std::string for every lookup
        struct SNameHash {
            using is_transparent = void;

            std::size_t operator()(std::string_view name) const
            {
                return std::hash<std::string_view> {}(name);
            }
        };

        using TileRegistry = std::unordered_map<std::string, STileData, SNameHash, std::equal_to<>>;

        // Every tile type, the builtin ones and those added at runtime. The
        // registry owns the names, and as its nodes never move, a view of a
        // name stays valid for as long as the program runs.
        static TileRegistry & registry()
        {
            // *INDENT-OFF* (keep astyle from ruining this beauty)
            static TileRegistry tiles = {
                { "void",        { U' ', 0x000000, true,  false } },
                { "wall",        { U'#', 0x888888, false, false } },
                { "corridor",    { U'.', 0xCCCCCC, true,  true } },
                { "grass",       { U'"', 0xA7CC7C, true,  true } },
                { "plant",       { U'&', 0x8DAD68, false, true } },
                { "tree",        { U'T', 0x4D9157, false, false } },
                { "room",        { U'.', 0xCCCCCC, true,  true } },
                { "door_closed", { U'+', 0xAA7744, false, false } },
                { "door_open",   { U'+', 0xAA7744, true,  true } },
                { "window",      { U'=', 0x88CCFF, true,  false } },
                { "water",       { U'~', 0x3399FF, true,  false } },
                { "well",        { U'o', 0x3399FF, true,  false } },
                { "bridge",      { U'=', 0x8B4513, true,  true } },
                { "rock",        { U'^', 0x555555, false, false } }
            };
            // *INDENT-ON*

            return tiles;
        }

    public:

        /**
         * @brief Checks whether a tile type is registered.
         *
         * Allows callers to verify if a tile archetype exists without throwing an exception.
         *
         * @param key Tile type identifier name to check.
         * @return True if registered in the tile dictionary.
         */
        [[nodiscard]]
        static bool has(std::string_view key)
        {
            return registry().contains(key);
        }

        /**
         * @brief Retrieves the registry entry for a tile type.
         *
         * @param key Tile type identifier name.
         * @return Pair containing the canonical name string and its template @ref STileData.
         * @throws std::invalid_argument If @p key is not registered.
         */
        [[nodiscard]]
        static const std::pair<const std::string, STileData>& get(std::string_view key)
        {
            auto it = registry().find(key);

            if (it == registry().end()) {
                throw std::invalid_argument("Unknown tile type: " + std::string(key));
            }

            return *it;
        }

        /**
         * @brief Registers a new tile archetype or updates an existing template at runtime.
         *
         * Existing tiles already placed on a map retain their current attributes;
         * newly placed or modified cells will use the updated @p data template.
         *
         * @param name Unique tile identifier name.
         * @param data Physical and visual properties of the tile template.
         */
        static void RegisterTile(std::string_view name, const STileData & data)
        {
            if (auto it = registry().find(name); it != registry().end()) {
                it->second = data;
            } else {
                registry().emplace(name, data);
            }
        }
    };

    /**
     * @brief Represents an individual map cell with visual and physical attributes.
     *
     * Stored in @ref CMap and @ref CMatrix. Each tile holds an archetype type name,
     * a Unicode glyph, an RGB color (subtly perturbed via jittering upon assignment),
     * a transparency flag, and a passability flag.
     */
    class CTile {

    private:

        std::string_view type;
        char32_t glyph{};
        std::uint32_t rgb_color{};
        bool transparent{};
        bool passable{};

    public:

        /**
         * @brief Default constructor creating an uninitialized empty tile.
         */
        CTile() = default;

        /**
         * @brief Constructs a tile by looking up its archetype in the registry.
         *
         * Applies subtle color jittering to the base RGB color to create natural variation.
         *
         * @param key Registered tile type name (e.g., `"wall"`, `"corridor"`).
         * @throws std::invalid_argument If @p key is not registered.
         */
        CTile(std::string_view key)
        {
            setType(key);
        }

        /**
         * @brief Constructs a tile with explicit data without applying color jittering.
         *
         * Useful when deserializing saved maps where exact color and properties must be preserved.
         *
         * @param key Registered tile type name.
         * @param data Exact tile data to assign.
         * @throws std::invalid_argument If @p key is not registered.
         */
        CTile(std::string_view key, const STileData & data)
            : type(CTileData::get(key).first), glyph(data.glyph), rgb_color(data.rgb_color),
              transparent(data.transparent), passable(data.passable)
        {
        }

        /**
         * @brief Replaces the tile's archetype, updating glyph, passability, transparency, and jittered color.
         * @param key Registered tile type name.
         * @throws std::invalid_argument If @p key is not registered.
         */
        void setType(std::string_view key)
        {
            const auto& [name, data] = CTileData::get(key);

            // Keep the registry's name, not the caller's, which may be a
            // temporary
            type = name;
            glyph = data.glyph;
            rgb_color = GetJitteredColor(data.rgb_color);
            transparent = data.transparent;
            passable = data.passable;
        }

        /**
         * @brief Returns the tile's archetype type name.
         * @return Stable string view of the type identifier.
         */
        [[nodiscard]] std::string_view getType() const
        {
            return type;
        }

        /**
         * @brief Returns the Unicode glyph representing this tile.
         * @return 32-bit Unicode code point (e.g., `U'#'`, `U'.'`).
         */
        [[nodiscard]] char32_t getGlyph() const
        {
            return glyph;
        }

        /**
         * @brief Sets the Unicode glyph representing this tile.
         * @param g 32-bit Unicode code point (e.g., `U'-'`, `U'|'`).
         */
        void setGlyph(char32_t g) noexcept
        {
            glyph = g;
        }

        /**
         * @brief Returns the packed RGB color value (`0x00RRGGBB`).
         */
        [[nodiscard]] std::uint32_t getColor() const
        {
            return rgb_color;
        }

        /**
         * @brief Checks if light and line of sight pass through this tile.
         * @return True if transparent (does not obstruct field of view).
         */
        [[nodiscard]] bool isTransparent() const
        {
            return transparent;
        }

        /**
         * @brief Checks if actors and entities can traverse this tile.
         * @return True if passable (walkable).
         */
        [[nodiscard]] bool isPassable() const
        {
            return passable;
        }

        /**
         * @brief Generates a unary predicate lambda matching tiles of a specific type.
         *
         * Convenient for use with @ref CMatrix::CountNeighbors.
         *
         * @param type Target tile type name to match.
         * @return Lambda returning `true` when passed a @ref CTile with matching type.
         */
        static auto ByType(std::string_view type)
        {
            return [t = std::string(type)](const CTile & cell) {
                return cell.getType() == t;
            };
        }
    };

} // export

} // namespace RL

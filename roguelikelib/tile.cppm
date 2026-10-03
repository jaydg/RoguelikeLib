//////////////////////////////////////////////////////////////////////////
// Map Tile
//////////////////////////////////////////////////////////////////////////

module;

import rl.randomness;
import std;

export module rl.tile;

namespace RL
{

export struct STileData {
    // A single Unicode code point
    char32_t glyph = U' ';
    std::uint32_t rgb_color = 0xFFFFFF;
    bool transparent = false;
    bool passable = false;
};

export {

    // Encode a glyph as UTF-8, for printing it to a terminal. Anything that
    // is not a valid code point comes out as the replacement character.
    std::string EncodeUTF8(char32_t glyph)
    {
        if (glyph > 0x10FFFF || (glyph >= 0xD800 && glyph <= 0xDFFF)) {
            glyph = U'\uFFFD';
        }

        std::string utf8;

        if (glyph < 0x80) {
            utf8 += static_cast<char>(glyph);
        } else if (glyph < 0x800) {
            utf8 += static_cast<char>(0xC0 | (glyph >> 6));
            utf8 += static_cast<char>(0x80 | (glyph & 0x3F));
        } else if (glyph < 0x10000) {
            utf8 += static_cast<char>(0xE0 | (glyph >> 12));
            utf8 += static_cast<char>(0x80 | ((glyph >> 6) & 0x3F));
            utf8 += static_cast<char>(0x80 | (glyph & 0x3F));
        } else {
            utf8 += static_cast<char>(0xF0 | (glyph >> 18));
            utf8 += static_cast<char>(0x80 | ((glyph >> 12) & 0x3F));
            utf8 += static_cast<char>(0x80 | ((glyph >> 6) & 0x3F));
            utf8 += static_cast<char>(0x80 | (glyph & 0x3F));
        }

        return utf8;
    }

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
                { "wall",        { U'#', 0x888888, false, false } },
                { "corridor",    { U'.', 0xCCCCCC, true,  true } },
                { "grass",       { U'"', 0xA7CC7C, true,  true } },
                { "plant",       { U'&', 0x8DAD68, false, true } },
                { "tree",        { U'T', 0x4D9157, false, false } },
                { "room",        { U'.', 0xCCCCCC, true,  true } },
                { "door_closed", { U'+', 0xAA7744, false, false } },
                { "door_open",   { U'+', 0xAA7744, true,  true } },
                { "water",       { U'~', 0x3399FF, true,  false } },
                { "bridge",      { U'=', 0x8B4513, true,  true } },
                { "rock",        { U'^', 0x555555, false, false } }
            };
            // *INDENT-ON*

            return tiles;
        }

    public:

        // The registry's entry for a tile type: its own copy of the name and
        // the data. Throws for a type that was never registered.
        [[nodiscard]]
        static const std::pair<const std::string, STileData>& get(std::string_view key)
        {
            auto it = registry().find(key);

            if (it == registry().end()) {
                throw std::invalid_argument("Unknown tile type: " + std::string(key));
            }

            return *it;
        }

        // Add a new tile type at runtime, or change an existing one. Tiles
        // already on a map keep the data they were set with.
        static void RegisterTile(std::string_view name, const STileData & data)
        {
            if (auto it = registry().find(name); it != registry().end()) {
                it->second = data;
            } else {
                registry().emplace(name, data);
            }
        }
    };

    class CTile {

    private:

        std::string_view type;
        char32_t glyph{};
        std::uint32_t rgb_color{};
        bool transparent{};
        bool passable{};

    public:

        CTile() = default;

        CTile(std::string_view key)
        {
            setType(key);
        }

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

        [[nodiscard]] std::string_view getType() const
        {
            return type;
        }

        [[nodiscard]] char32_t getGlyph() const
        {
            return glyph;
        }

        [[nodiscard]] std::uint32_t getColor() const
        {
            return rgb_color;
        }

        [[nodiscard]] bool isTransparent() const
        {
            return transparent;
        }

        [[nodiscard]] bool isPassable() const
        {
            return passable;
        }

        // predicate factory for CountNeighbors
        static auto ByType(std::string_view type)
        {
            return [t = std::string(type)](const CTile & cell) {
                return cell.getType() == t;
            };
        }
    };

} // export

} // namespace RL

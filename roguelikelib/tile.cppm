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

// Additional tiles, can be added at runtime
static std::unordered_map<std::string, STileData> additional_tiles;

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
        using TileDataEntry = std::map<std::string_view, STileData, std::less<>>;

        // Builtin standard tiles
        static const TileDataEntry & defaults()
        {
            // *INDENT-OFF* (keep astyle from ruining this beauty)
            static const TileDataEntry defaults = {
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

            return defaults;
        }

    public:

        [[nodiscard]]
        static const STileData * get(std::string_view key)
        {
            if (auto it = defaults().find(key); it != defaults().end()) {
                return &(it->second);
            }

            // Search in runtime data (O(1))
            auto dyn_it = additional_tiles.find(std::string(key));

            if (dyn_it != additional_tiles.end()) {
                return &(dyn_it->second);
            }

            return nullptr;
        }

        // Add new tile type at runtime
        static void RegisterTile(std::string name, STileData data)
        {
            additional_tiles[std::move(name)] = data;
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
            type = key;
            auto data = CTileData::get(key);

            glyph = data->glyph;
            rgb_color = GetJitteredColor(data->rgb_color);
            transparent = data->transparent;
            passable = data->passable;
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

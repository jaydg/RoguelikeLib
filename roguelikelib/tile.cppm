//////////////////////////////////////////////////////////////////////////
// Map Tile
//////////////////////////////////////////////////////////////////////////

module;

import rl.randomness;
import std;

export module rl.tile;

namespace RL {

// Additional tiles, can be added at runtime
struct STileData;
static std::unordered_map<std::string, STileData> additional_tiles;

struct STileData {
    char          glyph       = ' ';
    std::uint32_t rgb_color   = 0xFFFFFF;
    bool          transparent = false;
    bool          passable    = false;
};

export {

class CTileData {
private:
    using TileDataEntry = std::map<std::string_view, STileData, std::less<>>;

    // Builtin standard tiles
    static const TileDataEntry& defaults() {
        static const TileDataEntry defaults = {
            { "wall",        { '#', 0x888888, false, false } },
            { "corridor",    { '.', 0xCCCCCC, true,  true  } },
            { "grass",       { '"', 0xA7CC7C, true,  true  } },
            { "plant",       { '.', 0x8DAD68, false, true  } },
            { "room",        { '.', 0xCCCCCC, true,  true  } },
            { "door_closed", { '+', 0xAA7744, false, false } },
            { "door_open",   { '+', 0xAA7744, true,  true  } },
            { "water",       { '~', 0x3399FF, true,  false } }
        };

        return defaults;
    }

public:

    [[nodiscard]]
    static const STileData* get(std::string_view key)
    {
        if (auto it = defaults().find(key); it != defaults().end())
            return &(it->second);

        // Search in runtime data (O(1))
        auto dyn_it = additional_tiles.find(std::string(key));
        if (dyn_it != additional_tiles.end()) {
            return &(dyn_it->second);
        }

        return nullptr;
    }

    // Add new tile type at runtime
    void RegisterTile(std::string name, STileData data) {
        additional_tiles[std::move(name)] = data;
    }
};

class CTile {

private:

    std::string_view type;
    char glyph{};
    std::uint32_t rgb_color{};
    bool transparent{};
    bool passable{};

public:

    CTile() = default;

    CTile(std::string_view key) {
        setType(key);
    }

    void setType(std::string_view key) {
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

    [[nodiscard]] char getGlyph() const
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

};

} // export

} // namespace RL

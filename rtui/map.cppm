// Terminal rendering of a roguelike map.
module;

export module rtui.map;

export import rl.map;
export import rtui.screen;
import rl.tile;
import rtui.utf8;
import stc;
import std;

export namespace RL
{

// Viewport helpers adapted from Roguevalley's rv.view module.
// Centres the focus in the view, clamped to the map's edges. A map
// smaller than the view starts at the view's top left corner.
[[nodiscard]] Position CameraOrigin(Size map, Size view, Position focus)
{
    const auto axis = [](std::size_t map_length, std::size_t view_length, std::size_t at) {
        if (map_length <= view_length) {
            return std::size_t{0};
        }

        const std::size_t half = view_length / 2;
        return std::min(at > half ? at - half : 0, map_length - view_length);
    };

    return Position(axis(map.x, view.x, focus.x), axis(map.y, view.y, focus.y));
}

// Converts a map position to a screen position, or nothing outside the
// view. Origin is the map position at the area's top left corner.
[[nodiscard]] std::optional<Position> OnScreen(const SRect& area, Position origin, Position pos)
{
    if (pos.x < origin.x || pos.y < origin.y || pos.x - origin.x >= area.size.x || pos.y - origin.y >= area.size.y) {
        return std::nullopt;
    }

    return Position(area.corner.x + (pos.x - origin.x), area.corner.y + (pos.y - origin.y));
}

// Draws tiles from origin into area, using their glyphs and colours.
// Respects the screen's existing clip. Cells outside the map are left
// untouched; the caller can clear the screen or draw a background first.
void DrawMap(CScreen& screen, const CMap& map, const SRect& area, Position origin = Position(0, 0))
{
    const Size size = screen.getSize();

    if (origin.x >= map.getWidth() || origin.y >= map.getHeight() ||
            area.corner.x >= size.x || area.corner.y >= size.y) {
        return;
    }

    const std::size_t width = std::min({area.size.x, map.getWidth() - origin.x, size.x - area.corner.x});
    const std::size_t height = std::min({area.size.y, map.getHeight() - origin.y, size.y - area.corner.y});

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const auto& tile = map.get(origin.x + x, origin.y + y);
            screen.Put(area.corner.x + x, area.corner.y + y, tile.getGlyph(), tile.getColor());
        }
    }
}

void PrintMap(const CMap& map)
{
    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            auto tile = map.get(x, y);
            std::cout << stc::rgb_fg(tile.getColor()) << EncodeUTF8(tile.getGlyph());
        }

        std::cout << stc::reset << std::endl;
    }
}

} // namespace RL

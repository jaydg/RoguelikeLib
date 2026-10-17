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
[[nodiscard]] constexpr Position CameraOrigin(Size map, Size view, Position focus) noexcept
{
    const auto axis = [](std::size_t map_length, std::size_t view_length, std::size_t at) constexpr noexcept {
        if (map_length <= view_length) {
            return std::size_t{0};
        }

        const std::size_t half = view_length / 2;
        return std::min(at > half ? at - half : 0, map_length - view_length);
    };

    return Position(axis(map.x, view.x, focus.x), axis(map.y, view.y, focus.y));
}

// Converts a map position (pos_x, pos_y) to a screen position, or nothing
// outside the view. Origin is the map position at the area's top left corner.
[[nodiscard]] constexpr std::optional<Position> OnScreen(const SRect& area, Position origin, std::size_t pos_x, std::size_t pos_y) noexcept
{
    if (pos_x < origin.x || pos_y < origin.y || pos_x - origin.x >= area.size.x || pos_y - origin.y >= area.size.y) {
        return std::nullopt;
    }

    return Position(area.corner.x + (pos_x - origin.x), area.corner.y + (pos_y - origin.y));
}

// Converts a map position to a screen position, or nothing outside the
// view. Origin is the map position at the area's top left corner.
[[nodiscard]] constexpr std::optional<Position> OnScreen(const SRect& area, Position origin, Position pos) noexcept
{
    return OnScreen(area, origin, pos.x, pos.y);
}

// Converts a screen position (screen_x, screen_y) to a map position, or nothing
// if the screen position falls outside area. Origin is the map position at the
// area's top left corner.
[[nodiscard]] constexpr std::optional<Position> ToMap(const SRect& area, Position origin, std::size_t screen_x, std::size_t screen_y) noexcept
{
    if (!area.Contains(screen_x, screen_y)) {
        return std::nullopt;
    }

    return Position(origin.x + (screen_x - area.corner.x), origin.y + (screen_y - area.corner.y));
}

// Converts a screen position to a map position, or nothing if the screen
// position falls outside area. Origin is the map position at the area's
// top left corner.
[[nodiscard]] constexpr std::optional<Position> ToMap(const SRect& area, Position origin, Position screen_pos) noexcept
{
    return ToMap(area, origin, screen_pos.x, screen_pos.y);
}

// Draws a map of a size from origin into area, each position as `cell`
// says: the cell to put there, or nothing, which leaves the screen as it
// is there - for what is not known, say. It is asked only about positions
// on the map, in the area, and on the screen. Respects the screen's
// existing clip. What the map shows is up to the caller: light, what is
// remembered, who stands where.
template <typename CellFunc>
requires std::invocable<CellFunc, Position>
void DrawMap(CScreen& screen, Size map, const SRect& area, Position origin, CellFunc&& cell)
{
    if (origin.x >= map.x || origin.y >= map.y) {
        return;
    }

    const Size size = screen.getSize();
    SRect visible{area.corner, Size(std::min(area.size.x, map.x - origin.x), std::min(area.size.y, map.y - origin.y))};

    visible = visible.Intersection(SRect{Position(0, 0), size});
    if (visible.Empty()) {
        return;
    }

    if (const auto& clip = screen.getClip()) {
        visible = visible.Intersection(*clip);
        if (visible.Empty()) {
            return;
        }
    }

    const Position effective_origin(origin.x + (visible.corner.x - area.corner.x),
                                    origin.y + (visible.corner.y - area.corner.y));

    for (std::size_t y = 0; y < visible.size.y; ++y) {
        const std::size_t screen_y = visible.corner.y + y;
        const std::size_t map_y = effective_origin.y + y;
        const std::size_t screen_row = screen_y * size.x + visible.corner.x;

        for (std::size_t x = 0; x < visible.size.x; ++x) {
            if constexpr (std::same_as<std::remove_cvref_t<std::invoke_result_t<CellFunc, Position>>, SCell>) {
                const SCell shown = cell(Position(effective_origin.x + x, map_y));
                screen.PutUnchecked(screen_row + x, shown.glyph, shown.rgb, shown.bold, shown.inverse);
            } else {
                if (const auto shown = cell(Position(effective_origin.x + x, map_y))) {
                    screen.PutUnchecked(screen_row + x, shown->glyph, shown->rgb, shown->bold, shown->inverse);
                }
            }
        }
    }
}

// Draws tiles from origin into area, using their glyphs and colours.
// Respects the screen's existing clip. Cells outside the map are left
// untouched; the caller can clear the screen or draw a background first.
void DrawMap(CScreen& screen, const CMap& map, const SRect& area, Position origin = Position(0, 0))
{
    const Size size = screen.getSize();
    const Size map_size = map.getSize();

    if (origin.x >= map_size.x || origin.y >= map_size.y || area.corner.x >= size.x || area.corner.y >= size.y) {
        return;
    }

    const std::size_t width = std::min({area.size.x, map_size.x - origin.x, size.x - area.corner.x});
    const std::size_t height = std::min({area.size.y, map_size.y - origin.y, size.y - area.corner.y});

    const auto& clip = screen.getClip();
    if (!clip) {
        for (std::size_t y = 0; y < height; ++y) {
            const std::size_t screen_y = area.corner.y + y;
            const std::size_t map_y = origin.y + y;
            for (std::size_t x = 0; x < width; ++x) {
                const auto& tile = map(origin.x + x, map_y);
                screen.PutUnchecked(area.corner.x + x, screen_y, tile.getGlyph(), tile.getColor());
            }
        }
    } else {
        for (std::size_t y = 0; y < height; ++y) {
            const std::size_t screen_y = area.corner.y + y;
            const std::size_t map_y = origin.y + y;
            for (std::size_t x = 0; x < width; ++x) {
                const std::size_t screen_x = area.corner.x + x;
                if (clip->Contains(screen_x, screen_y)) {
                    const auto& tile = map(origin.x + x, map_y);
                    screen.PutUnchecked(screen_x, screen_y, tile.getGlyph(), tile.getColor());
                }
            }
        }
    }
}

void PrintMap(const CMap& map, std::ostream& out = std::cout)
{
    const std::size_t width = map.getWidth();
    const std::size_t height = map.getHeight();

    if (width == 0 || height == 0) {
        return;
    }

    std::string buffer;
    buffer.reserve(width * height * 4 + height * 8);

    std::optional<std::uint32_t> current_color;

    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const auto& tile = map(x, y);
            const std::uint32_t color = tile.getColor();

            if (color != current_color) {
                std::format_to(std::back_inserter(buffer), "\x1b[38;2;{};{};{}m",
                               (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
                current_color = color;
            }

            AppendUTF8(buffer, tile.getGlyph());
        }

        buffer += "\x1b[0m\n";
        current_color.reset();
    }

    out << buffer << std::flush;
}

} // namespace RL

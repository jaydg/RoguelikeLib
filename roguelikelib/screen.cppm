//////////////////////////////////////////////////////////////////////////
// Screen
//
// A grid of coloured glyphs that is drawn into and then sent to a
// terminal as a whole, as ANSI escape sequences with colours from stc.
//
// Text can carry its colours as markup: "<#ffd24a>" switches to a colour
// given outright, "<name>" to one registered with RegisterTag(), and "<<"
// is a literal '<'. Anything else in angle brackets stays as written.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.screen;

export import rl.position;
import rl.utf8;
import stc;
import std;

export namespace RL
{

// The colour of text that asks for none, and of empty cells
constexpr std::uint32_t default_text_colour = 0xC0C0C0;

// The colour at a percentage of its brightness, e.g. for what is
// remembered but out of sight
std::uint32_t Dim(std::uint32_t rgb, int percent)
{
    const auto scale = [percent](std::uint32_t channel) {
        return channel * static_cast<std::uint32_t>(std::clamp(percent, 0, 100)) / 100;
    };

    return (scale((rgb >> 16) & 0xFF) << 16) | (scale((rgb >> 8) & 0xFF) << 8) | scale(rgb & 0xFF);
}

// A colour moved towards another by a percentage: 0 is the first, 100 the
// second; e.g. to tint what a fire lights with the fire's colour
std::uint32_t Blend(std::uint32_t rgb, std::uint32_t towards, int percent)
{
    const auto weight = static_cast<std::uint32_t>(std::clamp(percent, 0, 100));
    const auto mix = [weight](std::uint32_t from, std::uint32_t to) {
        return (from * (100 - weight) + to * weight) / 100;
    };

    return (mix((rgb >> 16) & 0xFF, (towards >> 16) & 0xFF) << 16) | (mix((rgb >> 8) & 0xFF, (towards >> 8) & 0xFF) << 8) |
           mix(rgb & 0xFF, towards & 0xFF);
}

// A rectangle of cells: its top left corner and its size
struct SRect {
    Position corner{0, 0};
    Size size{0, 0};

    // The column and row just past it
    [[nodiscard]] std::size_t Right() const
    {
        return corner.x + size.x;
    }

    [[nodiscard]] std::size_t Bottom() const
    {
        return corner.y + size.y;
    }

    [[nodiscard]] bool Empty() const
    {
        return size.x == 0 || size.y == 0;
    }

    [[nodiscard]] bool Contains(Position pos) const
    {
        return pos.x >= corner.x && pos.y >= corner.y && pos.x < Right() && pos.y < Bottom();
    }

    // What it has in common with another
    [[nodiscard]] SRect Intersection(const SRect& other) const
    {
        const std::size_t left = std::max(corner.x, other.corner.x);
        const std::size_t top = std::max(corner.y, other.corner.y);
        const std::size_t right = std::min(Right(), other.Right());
        const std::size_t bottom = std::min(Bottom(), other.Bottom());

        if (right <= left || bottom <= top) {
            return {Position(left, top), Size(0, 0)};
        }

        return {Position(left, top), Size(right - left, bottom - top)};
    }

    // Smaller by a number of cells on every side
    [[nodiscard]] SRect Inset(std::size_t cells) const
    {
        if (size.x <= 2 * cells || size.y <= 2 * cells) {
            return {Position(corner.x + std::min(cells, size.x), corner.y + std::min(cells, size.y)), Size(0, 0)};
        }

        return {Position(corner.x + cells, corner.y + cells), Size(size.x - 2 * cells, size.y - 2 * cells)};
    }

    bool operator==(const SRect&) const = default;
};

struct SCell {
    char32_t glyph = U' ';
    std::uint32_t rgb = default_text_colour;

    bool operator==(const SCell&) const = default;
};

class CScreen
{
private:
    Size size = Size(0, 0);
    std::vector<SCell> cells;
    std::uint32_t text_colour = default_text_colour;
    std::map<std::string, std::uint32_t, std::less<>> tags;

    // Where drawing goes; anything outside is dropped
    std::optional<SRect> clip;

    // The colour a tag names, if it is one
    [[nodiscard]] std::optional<std::uint32_t> TagColour(std::string_view name) const
    {
        if (name.size() == 7 && name[0] == '#') {
            std::uint32_t rgb{};
            const auto[end, error] = std::from_chars(name.data() + 1, name.data() + name.size(), rgb, 16);

            if (error == std::errc() && end == name.data() + name.size()) {
                return rgb;
            }

            return std::nullopt;
        }

        if (const auto it = tags.find(name); it != tags.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    // Walks through text with markup and calls `put` with each glyph and
    // the colour it is to be drawn in
    template <typename Put>
    void ForEachGlyph(std::string_view markup, std::uint32_t rgb, Put&& put) const
    {
        std::size_t pos = 0;

        while (pos < markup.size()) {
            if (markup[pos] == '<') {
                if (markup.substr(pos, 2) == "<<") {
                    put(U'<', rgb);
                    pos += 2;
                    continue;
                }

                const std::size_t close = markup.find('>', pos + 1);

                if (close != std::string_view::npos) {
                    if (const auto tag = TagColour(markup.substr(pos + 1, close - pos - 1))) {
                        rgb = *tag;
                        pos = close + 1;
                        continue;
                    }
                }
            }

            const SDecodedUTF8 decoded = DecodeUTF8(markup.substr(pos));
            put(decoded.code_point, rgb);
            pos += decoded.length;
        }
    }

public:
    CScreen() = default;

    explicit CScreen(Size a_size)
    {
        Resize(a_size);
    }

    // Changes the size, clearing the screen if it is a different one
    void Resize(Size a_size)
    {
        if (a_size != size) {
            size = a_size;
            cells.assign(size.x * size.y, SCell{});
        }
    }

    void Clear()
    {
        std::ranges::fill(cells, SCell{});
    }

    [[nodiscard]] Size getSize() const
    {
        return size;
    }

    // Makes "<name>" in markup switch to a colour, or changes the colour
    // of a tag registered before
    void RegisterTag(std::string_view name, std::uint32_t rgb)
    {
        if (auto it = tags.find(name); it != tags.end()) {
            it->second = rgb;
        } else {
            tags.emplace(name, rgb);
        }
    }

    // The colour text starts in when Print() is given none
    void setTextColour(std::uint32_t rgb)
    {
        text_colour = rgb;
    }

    [[nodiscard]] std::uint32_t getTextColour() const
    {
        return text_colour;
    }

    // How many cells text with markup takes once printed
    [[nodiscard]] std::size_t TextWidth(std::string_view markup) const
    {
        std::size_t width = 0;
        ForEachGlyph(markup, 0, [&width](char32_t, std::uint32_t) {
            ++width;
        });

        return width;
    }

    [[nodiscard]] const SCell& At(std::size_t x, std::size_t y) const
    {
        if (x >= size.x || y >= size.y) {
            throw std::out_of_range(std::format("cell ({}, {}) is not on the screen", x, y));
        }

        return cells[y * size.x + x];
    }

    // Keeps drawing within a rectangle, e.g. a window's: what falls
    // outside it is dropped, as if it were off the screen
    void setClip(const SRect& rect)
    {
        clip = rect;
    }

    // Lets drawing go anywhere on the screen again
    void resetClip()
    {
        clip.reset();
    }

    // Draws a glyph; off the screen or outside the clip rectangle it is
    // dropped. Control codes are drawn as blanks, so that they cannot reach
    // the terminal.
    void Put(std::size_t x, std::size_t y, char32_t glyph, std::uint32_t rgb)
    {
        if (x < size.x && y < size.y && (!clip || clip->Contains(Position(x, y)))) {
            cells[y * size.x + x] = SCell{glyph < U' ' || glyph == U'\x7F' ? U' ' : glyph, rgb};
        }
    }

    // Draws text with markup from (x, y) to the right, cut off at the edge
    // of the screen or the clip rectangle. Returns the column after the text.
    std::size_t Print(std::size_t x, std::size_t y, std::string_view markup, std::uint32_t rgb)
    {
        ForEachGlyph(markup, rgb, [&](char32_t glyph, std::uint32_t colour) {
            Put(x++, y, glyph, colour);
        });

        return x;
    }

    std::size_t Print(std::size_t x, std::size_t y, std::string_view markup)
    {
        return Print(x, y, markup, text_colour);
    }

    // Everything needed to show the whole screen on a terminal, from the
    // top left corner. Colours go out as 24-bit values or as the nearest
    // of 256, and only when they change, which keeps a frame small enough
    // to be written at once.
    [[nodiscard]] std::string Compose(bool true_color) const
    {
        std::ostringstream out;
        out << (true_color ? stc::true_color : stc::color_256);

        std::optional<std::uint32_t> current;

        for (std::size_t y = 0; y < size.y; ++y) {
            out << "\x1b[" << y + 1 << ";1H";

            for (std::size_t x = 0; x < size.x; ++x) {
                const SCell& cell = cells[y * size.x + x];

                if (cell.glyph != U' ' && cell.rgb != current) {
                    out << stc::rgb_fg(cell.rgb);
                    current = cell.rgb;
                }

                out << EncodeUTF8(cell.glyph);
            }
        }

        out << stc::reset;

        return out.str();
    }
};

} // namespace RL

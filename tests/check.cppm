//////////////////////////////////////////////////////////////////////////
// What the tests share: checking and counting failures, maps drawn as
// text, and what a screen shows
//////////////////////////////////////////////////////////////////////////

module;

export module rltests.check;

export import rl.map;
export import rl.position;
export import rtui.screen;
import std;

namespace RL::tests
{

int checks = 0;
int failures = 0;

}

export namespace RL::tests
{

// Counts a check, and tells of it, with where it is, when it fails
void Check(bool condition, std::string_view what, std::source_location where = std::source_location::current())
{
    ++checks;

    if (!condition) {
        ++failures;
        std::cerr << where.file_name() << ':' << where.line() << ": FAILED: " << what << '\n';
    }
}

[[nodiscard]] int Checks()
{
    return checks;
}

[[nodiscard]] int Failures()
{
    return failures;
}

// A map drawn as text, one string per row, of the library's own tiles:
// '.' grass, ':' corridor, '=' bridge, '&' plant, 'T' tree, '^' rock,
// '~' water
CMap MakeMap(std::initializer_list<std::string_view> rows)
{
    const std::size_t height = rows.size();
    const std::size_t width = height > 0 ? rows.begin()->size() : 0;
    CMap map(Size(width, height));
    std::size_t y = 0;

    for (const std::string_view row : rows) {
        if (row.size() != width) {
            throw std::invalid_argument("all rows of a test map must be equally long");
        }

        for (std::size_t x = 0; x < width; ++x) {
            static const std::map<char, std::string_view> types = {
                {'.', "grass"},
                {':', "corridor"},
                {'=', "bridge"},
                {'&', "plant"},
                {'T', "tree"},
                {'^', "rock"},
                {'~', "water"}
            };

            map.SetCell(x, y, types.at(row[x]));
        }

        ++y;
    }

    return map;
}

// The characters of a row of a screen, without their colours
std::u32string Row(const CScreen& screen, std::size_t y)
{
    std::u32string row;

    for (std::size_t x = 0; x < screen.getSize().x; ++x) {
        row += screen.At(x, y).glyph;
    }

    return row;
}

// Whether text holds other text, or a character
bool Holds(std::u32string_view text, std::u32string_view part)
{
    return text.find(part) != std::u32string_view::npos;
}

bool Holds(std::u32string_view text, char32_t part)
{
    return text.find(part) != std::u32string_view::npos;
}

} // namespace RL::tests

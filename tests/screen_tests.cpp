// Tests for RTUI's rtui.screen and rtui.map: text with markup, drawn and
// wrapped; what goes out to the terminal; and maps drawn through a camera

import rtui.map;
import rtui.screen;
import rtui.utf8;
import rltests.check;
import std;

using RL::tests::Check;
using RL::tests::Row;

void TestScreen()
{
    RL::CScreen screen(RL::Size(12, 3));

    // Without tags registered, only colours given outright are markup
    screen.Print(0, 0, "<key>a<#102030>b");
    Check(Row(screen, 0) == U"<key>ab     ", "an unregistered tag stays as written");
    Check(screen.At(0, 0).rgb == RL::default_text_colour, "text starts in the default colour");
    Check(screen.At(6, 0).rgb == 0x102030, "a colour can be given as #rrggbb");
    Check(screen.TextWidth("<key>a") == 6, "an unregistered tag takes room");

    screen.RegisterTag("key", 0x00A6A6);
    screen.RegisterTag("key", 0x123456);
    screen.setTextColour(0xABCDEF);
    screen.Clear();
    const std::size_t end = screen.Print(1, 0, "a<key>b<#102030>c<<d<x>e");
    Check(Row(screen, 0) == U" abc<d<x>e  ", "markup is drawn as text and colours");
    Check(end == 10, "Print returns the column after the text");
    Check(screen.At(1, 0).rgb == 0xABCDEF, "text starts in the text colour");
    Check(screen.At(2, 0).rgb == 0x123456, "a tag changes the colour, as registered last");
    Check(screen.At(3, 0).rgb == 0x102030, "a colour given outright");
    Check(screen.At(4, 0).glyph == U'<' && screen.At(4, 0).rgb == 0x102030, "<< is a literal <");
    Check(screen.At(6, 0).glyph == U'<', "an unknown tag stays as written");
    Check(screen.Print(0, 1, "<#zzzzzz>") == 9, "a malformed colour stays as written");
    Check(screen.TextWidth("<key>Seed <#102030>42") == 7, "markup takes no room");
    Check(screen.TextWidth("ä♣") == 2, "a character in UTF-8 takes one cell");
    screen.Clear();

    screen.Print(9, 1, "clipped");
    Check(Row(screen, 1) == U"         cli", "text is cut off at the edge");

    screen.Put(0, 2, U'\x1b', 0);
    Check(screen.At(0, 2).glyph == U' ', "control codes never reach the terminal");
    screen.Put(50, 50, U'x', 0);   // dropped, not a crash

    // Direct AppendUTF8 test for ASCII and multibyte codepoints
    std::string utf8_buf;
    RL::AppendUTF8(utf8_buf, U'A');
    RL::AppendUTF8(utf8_buf, U'ä');
    RL::AppendUTF8(utf8_buf, U'♣');
    RL::AppendUTF8(utf8_buf, U'😀');
    Check(utf8_buf == "Aä♣😀", "AppendUTF8 correctly encodes ASCII and multibyte codepoints");

    RL::CScreen small(RL::Size(3, 1));
    small.Put(0, 0, U'♣', 0x00FF00, true, false);
    const std::string frame = small.Compose(true);
    Check(frame.find("\xE2\x99\xA3") != std::string::npos, "glyphs go out as UTF-8");
    Check(frame.find("\x1b[38;2;0;255;0m") != std::string::npos, "24-bit colour");
    Check(frame.find("\x1b[1m") != std::string::npos, "bold escape sequence emitted");
    Check(small.Compose(false).find("\x1b[38;5;") != std::string::npos, "256 colours");

    // Incremental compose when unchanged returns empty string
    Check(small.Compose(true, &small).empty(), "identical previous screen emits empty compose string");

    screen.Put(1, 0, U'a', 0);
    screen.Resize(RL::Size(12, 3));
    Check(screen.At(1, 0).glyph == U'a', "resizing to the same size keeps the content");
    screen.Resize(RL::Size(4, 2));
    Check(screen.getSize() == RL::Size(4, 2) && screen.At(0, 0).glyph == U' ', "resizing clears");

    // Dim and Blend tests (constexpr, noexcept, and runtime clamping)
    static_assert(noexcept(RL::Dim(0x8040FF, 50)));
    static_assert(noexcept(RL::Blend(0x000000, 0xFFFFFF, 50)));
    static_assert(RL::Dim(0x8040FF, 50) == 0x40207F);
    static_assert(RL::Dim(0xFFFFFF, 0) == 0x000000);
    static_assert(RL::Dim(0xFFFFFF, 100) == 0xFFFFFF);
    static_assert(RL::Blend(0x000000, 0x646464, 50) == 0x323232);
    static_assert(RL::Blend(0x102030, 0x506070, 0) == 0x102030);
    static_assert(RL::Blend(0x102030, 0x506070, 100) == 0x506070);

    Check(RL::Dim(0x8040FF, 50) == 0x40207F, "dimming scales every channel");
    Check(RL::Dim(0x8040FF, -20) == 0x000000, "dimming clamps negative percent to 0");
    Check(RL::Dim(0x8040FF, 150) == 0x8040FF, "dimming clamps percent > 100 to 100");
    Check(RL::Blend(0x000000, 0x646464, 50) == 0x323232, "blend mixes towards target colour");
    Check(RL::Blend(0x112233, 0x445566, -10) == 0x112233, "blend clamps negative percentage");
    Check(RL::Blend(0x112233, 0x445566, 150) == 0x445566, "blend clamps percentage above 100");

    // Text wraps between words, and its colour carries on across the break
    const auto lines = screen.WrapText("aaa bbb <key>ccc ddd", 7);
    Check(lines == std::vector<std::string> {"aaa bbb", "<key>ccc ddd"}, "words wrap at the width");
    Check(screen.WrapText("<key>aaa bbb ccc", 7) == std::vector<std::string> {"<key>aaa bbb", "<key>ccc"},
          "a colour carries on into the next line");
    Check(screen.WrapText("<x>aaa bbb ccc", 7) == std::vector<std::string> {"<x>aaa", "bbb ccc"},
          "what is no tag is text, and carries nothing on");
    Check(screen.WrapText("abcdefghij", 4) == std::vector<std::string> {"abcdefghij"}, "a word too long stays whole");
    Check(screen.WrapText("", 10).empty(), "nothing to wrap");
    Check(screen.WrapText("one   two three", 7) == std::vector<std::string> {"one two", "three"},
          "multiple spaces between words are handled correctly");
    Check(screen.WrapText("1 2 3 4 5 6", 3) == std::vector<std::string> {"1 2", "3 4", "5 6"},
          "tight budget wraps every two single-character words");
}

void TestCamera()
{
    using RL::CameraOrigin;
    const RL::Size map(100, 50);
    const RL::Size view(20, 10);

    Check(CameraOrigin(map, view, RL::Position(50, 25)) == RL::Position(40, 20), "the focus is in the middle");
    Check(CameraOrigin(map, view, RL::Position(3, 2)) == RL::Position(0, 0), "the top left corner stays put");
    Check(CameraOrigin(map, view, RL::Position(99, 49)) == RL::Position(80, 40), "the bottom right corner stays put");
    Check(CameraOrigin(RL::Size(10, 5), view, RL::Position(9, 4)) == RL::Position(0, 0),
          "a small map starts in the corner");
    Check(CameraOrigin(RL::Size(20, 10), view, RL::Position(15, 8)) == RL::Position(0, 0),
          "a map just as big as the view does not move");

    // Constexpr and noexcept verification
    static_assert(noexcept(CameraOrigin(map, view, RL::Position(50, 25))));
    static_assert(CameraOrigin(RL::Size(100, 50), RL::Size(20, 10), RL::Position(50, 25)) == RL::Position(40, 20));
    static_assert(CameraOrigin(RL::Size(10, 5), RL::Size(20, 10), RL::Position(9, 4)) == RL::Position(0, 0));

    const RL::SRect area{RL::Position(2, 1), RL::Size(5, 3)};
    static_assert(noexcept(RL::OnScreen(area, RL::Position(10, 10), RL::Position(12, 11))));
    static_assert(RL::OnScreen(RL::SRect{RL::Position(2, 1), RL::Size(5, 3)},
                               RL::Position(10, 10), RL::Position(12, 11)) == RL::Position(4, 2));
    static_assert(!RL::OnScreen(RL::SRect{RL::Position(2, 1), RL::Size(5, 3)},
                                RL::Position(10, 10), RL::Position(15, 11)).has_value());

    Check(RL::OnScreen(area, RL::Position(10, 10), RL::Position(12, 11)) == RL::Position(4, 2) &&
          !RL::OnScreen(area, RL::Position(10, 10), RL::Position(15, 11)), "a map position is on screen where the view shows it");

    // Drawn cell by cell as the caller says, the camera and the edges kept
    // by the library: only what is on the map, in the area and on the
    // screen is asked about, and nothing leaves the screen as it is
    RL::CScreen screen(RL::Size(8, 5));
    screen.Put(3, 2, U'x', 0);
    std::size_t asked = 0;
    RL::DrawMap(screen, RL::Size(4, 10), area, RL::Position(1, 1), [&](RL::Position pos) -> std::optional<RL::SCell> {
        ++asked;

        if (pos == RL::Position(2, 2))
        {
            return std::nullopt;
        }

        return RL::SCell{pos.x == 1 ? U'a' : U'b', 0x102030};
    });
    Check(asked == 9, "only positions on the map and in the area are asked about");
    Check(Row(screen, 1) == U"  abb   " && Row(screen, 2) == U"  axb   " && screen.At(2, 1).rgb == 0x102030,
          "each is drawn as the caller says, or left as it was");

    const RL::CMap tiles = RL::tests::MakeMap({"T.", "~^"});
    RL::CScreen whole(RL::Size(3, 3));
    RL::DrawMap(whole, tiles, RL::SRect{RL::Position(1, 1), RL::Size(2, 2)});
    Check(whole.At(1, 1).glyph == U'T' && whole.At(2, 2).glyph == U'^', "a map's own tiles are drawn as they look");
}

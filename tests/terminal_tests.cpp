// Tests for RTUI's rtui.utf8, rtui.terminal and rtui.mouse: UTF-8, escape
// sequences, and what the mouse reports

import rtui.terminal;
import rtui.utf8;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{

bool Decodes(std::string_view input, bool at_end, RL::SKey key, std::size_t length)
{
    const RL::SDecodedKey decoded = RL::DecodeKey(input, at_end);
    return decoded.key == key && decoded.length == length;
}

RL::SKey Char(char32_t ch)
{
    return {RL::EKey::Char, ch};
}

} // namespace

void TestUTF8()
{
    using RL::DecodeUTF8;
    const char32_t bad = RL::replacement_character;

    Check(DecodeUTF8("a").code_point == U'a' && DecodeUTF8("a").length == 1, "ASCII");
    Check(DecodeUTF8("\xC3\xA4x").code_point == U'ä' && DecodeUTF8("\xC3\xA4x").length == 2, "two bytes");
    Check(DecodeUTF8("\xE2\x99\xA3").code_point == U'♣', "three bytes");
    Check(DecodeUTF8("\xF0\x9F\x8C\xB2").code_point == U'🌲', "four bytes");

    const auto truncated = DecodeUTF8("\xE2\x99");
    Check(!truncated.complete && truncated.length == 2, "a truncated sequence is incomplete");

    Check(DecodeUTF8("\x80").code_point == bad && DecodeUTF8("\x80").length == 1, "a stray continuation byte");
    Check(DecodeUTF8("\xC0\xAF").code_point == bad, "an overlong encoding");
    Check(DecodeUTF8("\xED\xA0\x80").code_point == bad, "a surrogate");
    Check(DecodeUTF8("\xF4\x90\x80\x80").code_point == bad, "beyond Unicode");

    Check(RL::EncodeUTF8(U'a') == "a" && RL::EncodeUTF8(U'ä') == "\xC3\xA4" && RL::EncodeUTF8(U'♣') == "\xE2\x99\xA3" &&
          RL::EncodeUTF8(U'🌲') == "\xF0\x9F\x8C\xB2", "encoding");
    Check(RL::EncodeUTF8(static_cast<char32_t>(0xD800)) == "\xEF\xBF\xBD", "a surrogate encodes as the replacement character");

    for (const char32_t glyph : {
                U'a', U'ß', U'♣', U'🌲'
            }) {
        Check(DecodeUTF8(RL::EncodeUTF8(glyph)).code_point == glyph, "decoding undoes encoding");
    }

    const auto broken = DecodeUTF8("\xE2" "ab");
    Check(broken.code_point == bad && broken.length == 1, "a broken sequence resynchronises on the next byte");
}

void TestDecodeKey()
{
    using RL::EKey;

    Check(Decodes("\x1b[A", false, {RL::EKey::Up}, 3), "CSI up");
    Check(Decodes("\x1bOB", false, {RL::EKey::Down}, 3), "SS3 down");
    Check(Decodes("\x1b[1;5C", false, {RL::EKey::Right}, 6), "right with a modifier");
    Check(Decodes("\x1b[D", false, {RL::EKey::Left}, 3), "left");
    Check(Decodes("\x1b[H", false, {RL::EKey::Home}, 3) && Decodes("\x1b[1~", false, {RL::EKey::Home}, 4) &&
          Decodes("\x1b[7~", false, {RL::EKey::Home}, 4), "home");
    Check(Decodes("\x1b[F", false, {RL::EKey::End}, 3) && Decodes("\x1b[4~", false, {RL::EKey::End}, 4), "end");
    Check(Decodes("\x1b[5~", false, {RL::EKey::PageUp}, 4), "page up");
    Check(Decodes("\x1b[6~", false, {RL::EKey::PageDown}, 4), "page down");
    Check(Decodes("\x1b[3~", false, {RL::EKey::Delete}, 4), "delete");
    Check(Decodes("\x1b[E", false, {RL::EKey::Center}, 3) && Decodes("\x1b[G", false, {RL::EKey::Center}, 3), "keypad centre");

    Check(Decodes("\x1b[15~", false, {RL::EKey::None}, 5), "an unknown key is swallowed whole");
    Check(Decodes("\x1b[A\x1b[B", false, {RL::EKey::Up}, 3), "only the first of several keys");

    Check(Decodes("\x1b", false, {}, 0), "a lone Escape may be the start of a sequence");
    Check(Decodes("\x1b", true, {RL::EKey::Escape}, 1), "a lone Escape with nothing following");
    Check(Decodes("\x1b[1;", false, {}, 0), "an unfinished sequence waits");
    Check(Decodes("\x1b[1;", true, {RL::EKey::Escape}, 1), "an unfinished sequence that stays so is an Escape");
    RL::SKey alt_x{EKey::Char, U'x'};
    alt_x.alt = true;
    Check(Decodes("\x1bx", false, alt_x, 2), "Alt+letter is one key with its modifier");
    RL::SKey alt_right{EKey::Right};
    alt_right.alt = true;
    Check(Decodes("\x1b[1;3C", false, alt_right, 6), "CSI Alt+arrow retains its modifier");
    Check(Decodes("\x1b\x1b[C", false, alt_right, 4), "Escape-prefixed Alt+arrow retains its modifier");
    Check(Decodes("\x1b[1;7C", false, alt_right, 6), "Alt+Ctrl+arrow retains Alt");
    Check(Decodes("\x1b\xC3", false, {}, 0), "Alt+UTF-8 waits for a complete character");

    Check(Decodes("\r", false, {RL::EKey::Enter}, 1) && Decodes("\n", false, {RL::EKey::Enter}, 1), "enter");
    Check(Decodes("\x7f", false, {RL::EKey::Backspace}, 1) && Decodes("\x08", false, {RL::EKey::Backspace}, 1), "backspace");
    Check(Decodes("\x03", false, Char(U'\x03'), 1), "Ctrl-C");
    Check(Decodes("k", false, Char(U'k'), 1), "a letter");
    Check(Decodes("\xC3\xA4", false, Char(U'ä'), 2), "a letter in UTF-8");
    Check(Decodes("\xC3", false, {}, 0), "half a letter waits for the rest");
    Check(Decodes("\x80", false, {RL::EKey::None}, 1), "a broken byte is swallowed");

    // The mouse, as terminals report it in the SGR format
    const auto mouse = [](RL::EMouseAction action, RL::EMouseButton button, std::size_t x, std::size_t y) {
        RL::SMouse report;
        report.action = action;
        report.button = button;
        report.position = RL::Position(x, y);
        return RL::SKey{RL::EKey::Mouse, 0, report};
    };
    using RL::EMouseAction;
    using RL::EMouseButton;

    Check(Decodes("\x1b[<0;10;5M", false, mouse(EMouseAction::Press, EMouseButton::Left, 9, 4), 10),
          "a button pressed, at a cell counted from 0");
    Check(Decodes("\x1b[<0;10;5m", false, mouse(EMouseAction::Release, EMouseButton::Left, 9, 4), 10), "and released");
    Check(Decodes("\x1b[<34;120;40M", false, mouse(EMouseAction::Drag, EMouseButton::Right, 119, 39), 13),
          "dragging with a button held");
    Check(Decodes("\x1b[<64;1;1M", false, mouse(EMouseAction::ScrollUp, EMouseButton::None, 0, 0), 10) &&
          Decodes("\x1b[<65;1;1M", false, mouse(EMouseAction::ScrollDown, EMouseButton::None, 0, 0), 10), "the wheel");
    Check(Decodes("\x1b[<0;10", false, {}, 0), "half a mouse report waits for the rest");
    Check(Decodes("\x1b[<0;0;5M", false, {RL::EKey::None}, 9) && Decodes("\x1b[<0;1;1;1M", false, {RL::EKey::None}, 11),
          "a report that makes no sense is swallowed whole");
    Check(Decodes("\x1b[<0;2;3M\x1b[A", false, mouse(EMouseAction::Press, EMouseButton::Left, 1, 2), 9),
          "a key after a report is left for next time");
}

void TestMouse()
{
    using RL::EMouseAction;
    using RL::EMouseButton;
    using RL::ParseSGRMouse;

    const auto modified = ParseSGRMouse("29;3;4", 'M');
    Check(modified && modified->action == EMouseAction::Press && modified->button == EMouseButton::Middle &&
          modified->shift && modified->alt && modified->ctrl && modified->position == RL::Position(2, 3),
          "Shift, Alt and Ctrl are reported with the button");
    Check(ParseSGRMouse("35;3;4", 'M')->action == EMouseAction::Move, "moving with no button held");
    Check(ParseSGRMouse("3;3;4", 'M')->action == EMouseAction::Release, "released, as older reports say it");
    Check(!ParseSGRMouse("66;3;4", 'M') && !ParseSGRMouse("128;3;4", 'M'), "the wheel sideways and more buttons are not");
    Check(!ParseSGRMouse("0;3;4", 'X') && !ParseSGRMouse("0;3", 'M') && !ParseSGRMouse("0;3;4;5", 'M') &&
          !ParseSGRMouse("0;3;4;", 'M') && !ParseSGRMouse("", 'M') && !ParseSGRMouse("0;3;0", 'M'),
          "nor what is not a report");
}

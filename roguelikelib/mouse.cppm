//////////////////////////////////////////////////////////////////////////
// Mouse
//
// What a terminal reports of the mouse, once asked to: buttons pressed and
// released, dragging, and the wheel, each with the cell it happened in.
// Terminals report it in the SGR format of xterm, which kitty, GNOME
// Terminal, Konsole, WezTerm, Alacritty, tmux and others all speak:
//
// ESC [ < button ; column ; row M     pressed, dragged or wheel turned
// ESC [ < button ; column ; row m     released
//
// Reading the input is the caller's business; ParseSGRMouse() makes sense
// of what is between "ESC [ <" and the final M or m.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.mouse;

export import rl.position;
import std;

export namespace RL
{

// Has the terminal report buttons pressed and released and the mouse
// dragged with a button held, in the SGR format; and stop again
constexpr std::string_view mouse_reporting_on = "\x1b[?1002h\x1b[?1006h";
constexpr std::string_view mouse_reporting_off = "\x1b[?1006l\x1b[?1002l";

enum class EMouseButton {
    None,
    Left,
    Middle,
    Right
};

enum class EMouseAction {
    Press,
    Release,
    Drag,       // moved with a button held
    Move,       // moved with no button held, if the terminal reports that
    ScrollUp,
    ScrollDown
};

struct SMouse {
    EMouseAction action = EMouseAction::Press;
    EMouseButton button = EMouseButton::None;

    // The cell, from the top left corner of the terminal at (0, 0)
    Position position{0, 0};

    bool shift = false;
    bool alt = false;
    bool ctrl = false;

    bool operator==(const SMouse&) const = default;
};

// What a mouse report in the SGR format says: `parameters` is what comes
// between "ESC [ <" and the final byte, `final` that byte. Nothing if it
// is not a report, or one of something no mouse does.
std::optional<SMouse> ParseSGRMouse(std::string_view parameters, char final)
{
    if (final != 'M' && final != 'm') {
        return std::nullopt;
    }

    std::array<unsigned, 3> numbers{};
    std::size_t count = 0;

    for (std::size_t pos = 0; pos <= parameters.size(); ++count) {
        const std::size_t end = std::min(parameters.find(';', pos), parameters.size());

        if (count == numbers.size()) {
            return std::nullopt;
        }

        const auto[last, error] = std::from_chars(parameters.data() + pos, parameters.data() + end, numbers[count]);

        if (error != std::errc() || last != parameters.data() + end) {
            return std::nullopt;
        }

        pos = end + 1;
    }

    // Columns and rows count from 1
    if (count != numbers.size() || numbers[1] == 0 || numbers[2] == 0) {
        return std::nullopt;
    }

    const unsigned code = numbers[0];
    SMouse mouse;
    mouse.position = Position(numbers[1] - 1, numbers[2] - 1);
    mouse.shift = (code & 4) != 0;
    mouse.alt = (code & 8) != 0;
    mouse.ctrl = (code & 16) != 0;

    // The low two bits are the button: left, middle, right, or none
    static constexpr std::array buttons = {
        EMouseButton::Left, EMouseButton::Middle, EMouseButton::Right, EMouseButton::None
    };
    const EMouseButton button = buttons[code & 3];

    if ((code & 64) != 0) {
        // The wheel; sideways it is nothing the game knows
        if ((code & 3) > 1 || final == 'm') {
            return std::nullopt;
        }

        mouse.action = (code & 3) == 0 ? EMouseAction::ScrollUp : EMouseAction::ScrollDown;
        return mouse;
    }

    if ((code & 128) != 0) {
        // Buttons beyond the wheel
        return std::nullopt;
    }

    mouse.button = button;

    if (final == 'm') {
        mouse.action = EMouseAction::Release;
    } else if ((code & 32) != 0) {
        mouse.action = button == EMouseButton::None ? EMouseAction::Move : EMouseAction::Drag;
    } else if (button == EMouseButton::None) {
        // Released, as older reports say it; in the SGR format, 'm' does
        mouse.action = EMouseAction::Release;
    } else {
        mouse.action = EMouseAction::Press;
    }

    return mouse;
}

} // namespace RL

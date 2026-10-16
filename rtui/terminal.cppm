//////////////////////////////////////////////////////////////////////////
// The terminal: raw keyboard and mouse input and output, on anything POSIX
//
// Output is plain ANSI escape sequences, colours come from stc (see
// rtui.screen). Input is read byte by byte from a terminal in raw mode and
// decoded here, escape sequences and mouse reports included.
//////////////////////////////////////////////////////////////////////////

module;

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

export module rtui.terminal;

export import rtui.mouse;
import rl.position;
import rtui.utf8;
import std;

export namespace RL
{

enum class EKey {
    None,       // nothing the caller knows, swallowed
    Char,       // a character, or a control code below 0x20
    Enter,
    Escape,
    Backspace,
    Delete,
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    PageUp,
    PageDown,
    Center,     // keypad 5 with NumLock off
    Resize,     // the terminal changed size
    Mouse,      // something done with the mouse
    Closed      // the terminal is gone, nothing more will come
};

struct SKey {
    EKey type = EKey::None;
    char32_t ch = 0;

    // What was done with the mouse, for EKey::Mouse
    RL::SMouse mouse{};

    bool alt = false;

    bool operator==(const SKey&) const = default;
};

struct SDecodedKey {
    SKey key;

    // Bytes taken from the input. Zero when the input ends inside an
    // escape sequence that may still be completed.
    std::size_t length = 0;
};

// Decodes the first key; empty input consumes no bytes. With `at_end` set nothing
// more is coming, so an unfinished escape sequence is decoded as a plain
// Escape: the bytes after it then decode as keys of their own.
SDecodedKey DecodeKey(std::string_view input, bool at_end)
{
    if (input.empty()) {
        return {};
    }

    const auto byte = static_cast<unsigned char>(input[0]);

    if (byte == 0x1B) {
        if (input.size() == 1) {
return at_end ? SDecodedKey{{EKey::Escape}, 1} :
            SDecodedKey{};
        }

        if (input[1] != '[' && input[1] != 'O') {
            // Alt prefixes a normal key (or another escape sequence).
            auto decoded = DecodeKey(input.substr(1), at_end);
            if (decoded.length == 0) return {};
            decoded.key.alt = true;
            ++decoded.length;
            return decoded;
        }

        // CSI (ESC [) or SS3 (ESC O): parameters, then a final byte that
        // names the key. The second parameter encodes modifiers.
        int parameter = 0;
        int modifier = 0;
        bool first = true;
        bool second = true;
        std::size_t index = 2;

        for (; index < input.size(); ++index) {
            const char c = input[index];

            if (c >= '0' && c <= '9') {
                // Clamped: a longer number is nonsense, and must not
                // overflow
                if (first && parameter < 1000) {
                    parameter = parameter * 10 + (c - '0');
                } else if (!first && second && modifier < 1000) {
                    modifier = modifier * 10 + (c - '0');
                }
            } else if (c == ';') {
                if (!first) second = false;
                first = false;
            } else if (c < 0x20 || c > 0x3F) {
                // Not a parameter byte: the final one, or something
                // that cannot be part of a sequence at all
                break;
            }
        }

        if (index >= input.size()) {
return at_end ? SDecodedKey{{EKey::Escape}, 1} :
            SDecodedKey{};
        }

        EKey type = EKey::None;

        switch (input[index]) {
        case 'A':
            type = EKey::Up;
            break;

        case 'B':
            type = EKey::Down;
            break;

        case 'C':
            type = EKey::Right;
            break;

        case 'D':
            type = EKey::Left;
            break;

        case 'E':
        case 'G':
            type = EKey::Center;
            break;

        case 'H':
            type = EKey::Home;
            break;

        case 'F':
            type = EKey::End;
            break;

        case '~':
            switch (parameter) {
            case 1:
            case 7:
                type = EKey::Home;
                break;

            case 3:
                type = EKey::Delete;
                break;

            case 4:
            case 8:
                type = EKey::End;
                break;

            case 5:
                type = EKey::PageUp;
                break;

            case 6:
                type = EKey::PageDown;
                break;

            default:
                break;
            }

            break;

        default:
            break;
        }

        // A mouse report: ESC [ < button ; column ; row, then M or m
        if (input[1] == '[' && input[2] == '<') {
            const auto mouse = RL::ParseSGRMouse(input.substr(3, index - 3), input[index]);
            return {mouse ? SKey{EKey::Mouse, 0, mouse.value()} : SKey{}, index + 1};
        }

        // A sequence for an unsupported key - a function key -
        // is swallowed whole rather than handed out byte by byte
        SKey key{type};
        key.alt = modifier > 0 && ((modifier - 1) & 2) != 0;
        return {key, index + 1};
    }

    if (byte == '\r' || byte == '\n') {
        return {{EKey::Enter}, 1};
    }

    if (byte == 0x7F || byte == 0x08) {
        return {{EKey::Backspace}, 1};
    }

    if (byte < 0x20) {
        return {{EKey::Char, byte}, 1};
    }

    const RL::SDecodedUTF8 decoded = RL::DecodeUTF8(input);

    if (!decoded.complete) {
return at_end ? SDecodedKey{{EKey::None}, decoded.length} :
        SDecodedKey{};
    }

    if (decoded.code_point == RL::replacement_character) {
        return {{EKey::None}, decoded.length};
    }

    return {{EKey::Char, decoded.code_point}, decoded.length};
}

} // namespace RL

namespace RL
{

// How long to wait for the rest of an escape sequence before taking it for
// a lone Escape. A terminal sends a sequence all at once; a person cannot
// press another key that quickly.
constexpr int escape_delay_ms = 40;

constexpr std::string_view enter_sequence = "\x1b[?1049h\x1b[?25l\x1b[2J";
constexpr std::string_view leave_sequence = "\x1b[0m\x1b[?25h\x1b[?1049l";

// Writes everything. In the browser the write goes out to xterm.js, and
// cannot be interrupted.
bool WriteAll(std::string_view text)
{
    while (!text.empty()) {
        const ssize_t written = write(STDOUT_FILENO, text.data(), text.size());

        if (written < 0) {
#ifdef __EMSCRIPTEN__
            return false;
#else
            if (errno == EINTR) {
                continue;
            }

            return false;
#endif
        }

        if (written == 0) {
            errno = EIO;
            return false;
        }

        text.remove_prefix(static_cast<std::size_t>(written));
    }

    return true;
}

#ifndef __EMSCRIPTEN__

// Signals that would end the program with the terminal left raw
constexpr std::array fatal_signals = {SIGHUP, SIGINT, SIGQUIT, SIGTERM};

// Restoring the terminal has to work from a signal handler and from
// atexit() as well, so what it needs lives here rather than in the class.
termios original_mode{};
volatile sig_atomic_t raw_mode = 0;
volatile sig_atomic_t resized = 0;

// Puts the terminal back the way it was found. Safe to call more than once,
// and from a signal handler.
void RestoreTerminal()
{
    if (raw_mode) {
        raw_mode = 0;
        WriteAll(RL::mouse_reporting_off);
        WriteAll(leave_sequence);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_mode);
    }
}

void RestoreAtExit()
{
    RestoreTerminal();
}

void OnResize(int)
{
    resized = 1;
}

// Restores the terminal, then lets the signal do what it would have done
void OnFatalSignal(int signal_number)
{
    RestoreTerminal();
    signal(signal_number, SIG_DFL);
    raise(signal_number);
}

#endif // !__EMSCRIPTEN__

} // namespace RL

export namespace RL
{

// The terminal in raw mode on the alternate screen, for as long as this
// object lives. There can only be one at a time.
class CTerminal
{
private:
    std::string pending;
    bool closed = false;
#ifndef __EMSCRIPTEN__
    struct sigaction old_resize_action {};
    std::array<struct sigaction, fatal_signals.size()> old_fatal_actions{};
#endif

    enum class EWait { Ready, Timeout, Interrupted };

#ifndef __EMSCRIPTEN__
    static EWait WaitForInput(int timeout_ms)
    {
        pollfd descriptor{STDIN_FILENO, POLLIN, 0};
        const int result = poll(&descriptor, 1, timeout_ms);

        if (result < 0) {
            if (errno == EINTR) {
                return EWait::Interrupted;
            }

            throw std::system_error(errno, std::generic_category(), "waiting for the terminal");
        }

        return result > 0 ? EWait::Ready : EWait::Timeout;
    }

    void ReadAvailable()
    {
        char buffer[256];
        const ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (count > 0) {
            pending.append(buffer, static_cast<std::size_t>(count));
        } else if (count == 0) {
            closed = true;
        } else if (errno != EINTR) {
            throw std::system_error(errno, std::generic_category(), "reading the terminal");
        }
    }

public:
    // Opt in to movement reports even when no mouse button is held.
    explicit CTerminal(bool report_hover = false)
    {
        if (raw_mode) {
            throw std::logic_error("the terminal is already in use");
        }

        if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
            throw std::runtime_error("not running in an interactive terminal");
        }

        if (tcgetattr(STDIN_FILENO, &original_mode) != 0) {
            throw std::system_error(errno, std::generic_category(), "reading the terminal mode");
        }

        // Keys one at a time, unechoed, Ctrl-C and Ctrl-Z included. Output
        // processing stays on, so "\n" still starts a new line.
        termios raw = original_mode;
        raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_iflag &= ~static_cast<tcflag_t>(BRKINT | ICRNL | INLCR | ISTRIP | IXON);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;

        // Without SA_RESTART, so that a resize interrupts the wait for a key
        struct sigaction action {};
        sigemptyset(&action.sa_mask);
        action.sa_handler = OnResize;
        sigaction(SIGWINCH, &action, &old_resize_action);

        action.sa_handler = OnFatalSignal;

        for (std::size_t index = 0; index < fatal_signals.size(); ++index) {
            sigaction(fatal_signals[index], &action, &old_fatal_actions[index]);
        }

        // Also when the program leaves through std::exit()
        static const bool registered = atexit(RestoreAtExit) == 0;
        static_cast<void>(registered);

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
            const int error = errno;
            RestoreSignals();
            throw std::system_error(error, std::generic_category(), "setting the terminal mode");
        }

        raw_mode = 1;
        resized = 0;

        if (!WriteAll(enter_sequence) || !WriteAll(report_hover ? RL::mouse_hover_reporting_on : RL::mouse_reporting_on)) {
            const int error = errno;
            RestoreTerminal();
            RestoreSignals();
            throw std::system_error(error, std::generic_category(), "initializing the terminal");
        }
    }

    CTerminal(const CTerminal&) = delete;
    CTerminal& operator=(const CTerminal&) = delete;

    ~CTerminal()
    {
        RestoreTerminal();
        RestoreSignals();
    }

    void RestoreSignals()
    {
        sigaction(SIGWINCH, &old_resize_action, nullptr);

        for (std::size_t index = 0; index < fatal_signals.size(); ++index) {
            sigaction(fatal_signals[index], &old_fatal_actions[index], nullptr);
        }
    }

    // Columns and rows; 80x24 when the terminal will not say
    [[nodiscard]] static RL::Size getSize()
    {
        winsize size{};

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 && size.ws_row > 0) {
            return RL::Size(size.ws_col, size.ws_row);
        }

        return RL::Size(80, 24);
    }

#else // __EMSCRIPTEN__

    // In the browser the terminal is xterm.js: its host feeds the keys
    // through a pipe that read() drains, and reads the escape sequences
    // written back. Nothing is done about the mode of the terminal - the
    // browser terminal is raw already - and there are no signals.
    static EWait WaitForInput(int timeout_ms)
    {
        pollfd descriptor{STDIN_FILENO, POLLIN, 0};
        const int result = poll(&descriptor, 1, timeout_ms);

        if (result < 0) {
            throw std::system_error(errno, std::generic_category(), "waiting for the terminal");
        }

        return result > 0 ? EWait::Ready : EWait::Timeout;
    }

    void ReadAvailable()
    {
        char buffer[256];
        const ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (count > 0) {
            pending.append(buffer, static_cast<std::size_t>(count));
        } else if (count == 0) {
            closed = true;
        } else {
            throw std::system_error(errno, std::generic_category(), "reading the terminal");
        }
    }

public:
    // Opt in to movement reports even when no mouse button is held.
    explicit CTerminal(bool report_hover = false)
    {
        if (!WriteAll(enter_sequence) || !WriteAll(report_hover ? RL::mouse_hover_reporting_on : RL::mouse_reporting_on)) {
            throw std::system_error(errno, std::generic_category(), "initializing the terminal");
        }
    }

    CTerminal(const CTerminal&) = delete;
    CTerminal& operator=(const CTerminal&) = delete;

    ~CTerminal()
    {
        WriteAll(RL::mouse_reporting_off);
        WriteAll(leave_sequence);
    }

    void RestoreSignals() {}

    // Columns and rows, as the host says: it sets RV_COLUMNS and RV_ROWS
    // from the size of the xterm.js element before the program starts
    [[nodiscard]] static RL::Size getSize()
    {
        const char* columns = std::getenv("RV_COLUMNS");
        const char* rows = std::getenv("RV_ROWS");
        const int width = columns ? std::atoi(columns) : 0;
        const int height = rows ? std::atoi(rows) : 0;

        if (width > 0 && height > 0) {
            return RL::Size(static_cast<std::size_t>(width), static_cast<std::size_t>(height));
        }

        return RL::Size(80, 24);
    }

#endif // __EMSCRIPTEN__

    void Write(std::string_view text)
    {
        if (!WriteAll(text)) {
            throw std::system_error(errno, std::generic_category(), "writing to the terminal");
        }
    }

    // Waits for the next key. A resize is reported as a key of its own,
    // so that whoever waits can lay the screen out again. Given a time to
    // wait, in milliseconds, it waits no longer: then it reports a key of
    // the type None, e.g. to draw the next frame of what moves on screen.
    SKey ReadKey(int timeout_ms = -1)
    {
        for (;;) {
#ifndef __EMSCRIPTEN__
            if (resized) {
                resized = 0;
                return {EKey::Resize};
            }
#endif

            if (!pending.empty()) {
                SDecodedKey decoded = DecodeKey(pending, closed);

                if (decoded.length == 0) {
                    // The rest of a sequence may still be on its way
                    const EWait wait = WaitForInput(escape_delay_ms);

                    if (wait == EWait::Ready) {
                        ReadAvailable();
                        continue;
                    }

                    if (wait == EWait::Interrupted) {
                        continue;
                    }

                    decoded = DecodeKey(pending, true);
                }

                pending.erase(0, decoded.length);

                if (decoded.key.type != EKey::None) {
                    return decoded.key;
                }

                continue;
            }

            if (closed) {
                return {EKey::Closed};
            }

            const EWait wait = WaitForInput(timeout_ms);

            if (wait == EWait::Ready) {
                ReadAvailable();
            } else if (wait == EWait::Timeout) {
                return {EKey::None};
            }
        }
    }
};

} // namespace RL

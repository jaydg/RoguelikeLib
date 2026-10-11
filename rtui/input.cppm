//////////////////////////////////////////////////////////////////////////
// Input windows
//
// A window to type a line of text into, a name, say: after a prompt, what
// is typed so far, and where the next character goes. Typed characters are
// added, Backspace takes the last away, Enter enters the text and Escape
// cancels; what was entered, or that it was cancelled, waits until the
// caller takes it. The prompt is text with colour markup, as the screen
// prints it; what is typed is shown as it is.
//////////////////////////////////////////////////////////////////////////

module;

export module rtui.input;

export import rtui.windows;
import rtui.utf8;
import std;

export namespace RL
{

class CInputWindow : public CWindow
{
private:
    std::string prompt;
    std::u32string typed;
    std::size_t longest = 40;
    std::optional<std::string> entered;
    bool cancelled = false;

public:
    // A window that floats at first where it is given, which takes text up
    // to so many characters long
    CInputWindow(std::string title, const SRect& floating, std::size_t a_longest = 40,
                 EWindowOptions options = EWindowOptions::None)
        : CWindow(std::move(title), floating, Size(12, 3), options), longest(a_longest)
    {
    }

    // What is asked for, before what is typed, as markup
    void setPrompt(std::string a_prompt)
    {
        prompt = std::move(a_prompt);
    }

    // What is typed so far, as UTF-8; set, it starts from there, and
    // nothing is entered or cancelled yet
    [[nodiscard]] std::string getText() const
    {
        std::string text;

        for (const char32_t c : typed) {
            text += EncodeUTF8(c);
        }

        return text;
    }

    void setText(std::string_view text)
    {
        typed.clear();

        while (!text.empty() && typed.size() < longest) {
            const SDecodedUTF8 decoded = DecodeUTF8(text);
            typed += decoded.code_point;
            text.remove_prefix(decoded.length);
        }

        entered.reset();
        cancelled = false;
    }

    void Draw(CScreen& screen, const SRect& inside) override
    {
        std::string shown;

        for (const char c : getText()) {
            shown += c == '<' ? std::string("<<") : std::string(1, c);
        }

        const std::size_t end = screen.Print(inside.corner.x, inside.corner.y, prompt);
        const std::size_t after = screen.Print(end, inside.corner.y, shown);
        screen.Put(after, inside.corner.y, U'_', screen.getTextColour(), true);
    }

    // A character typed is added, Backspace takes the last away, Enter
    // enters the text, Escape cancels
    bool HandleKey(const SKey& key) override
    {
        switch (key.type) {
        case EKey::Char:
            if (key.ch < U' ' || key.ch == U'\x7F') {
                return false;
            }

            if (typed.size() < longest) {
                typed += key.ch;
            }

            return true;

        case EKey::Backspace:
            if (!typed.empty()) {
                typed.pop_back();
            }

            return true;

        case EKey::Enter:
            entered = getText();
            return true;

        case EKey::Escape:
            cancelled = true;
            return true;

        default:
            return false;
        }
    }

    // What was entered, if it was since it was taken last
    [[nodiscard]] std::optional<std::string> TakeEntered()
    {
        return std::exchange(entered, std::nullopt);
    }

    // Whether it was cancelled, since that was taken last
    [[nodiscard]] bool TakeCancelled()
    {
        return std::exchange(cancelled, false);
    }
};

} // namespace RL

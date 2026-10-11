//////////////////////////////////////////////////////////////////////////
// Text windows
//
// A window of lines of text, which may be more than it has rows: it shows
// them from the first it is scrolled to, and tells the window manager how
// far that is, which shows it on the frame and lets it be dragged there.
// The wheel scrolls it. Which lines it shows, wrapped to its width, a
// window derived from it says; one that follows the latest, like a log of
// messages, stays at the last line until scrolled back from it.
//
// A log of messages, for instance, derives from CTextWindow, answers its
// messages from Lines(), and is made following the latest.
//////////////////////////////////////////////////////////////////////////

module;

export module rtui.text;

export import rtui.windows;
import std;

export namespace RL
{

class CTextWindow : public CWindow
{
private:
    // Whether it stays at the last line, while it is scrolled to it
    bool follows = false;

    // The first line shown, how many there are, and how much room they
    // had, as drawn last
    std::size_t first = 0;
    std::size_t lines = 0;
    Size view{0, 0};

    // The first line shown when scrolled as far down as it goes
    [[nodiscard]] std::size_t Last() const
    {
        return lines > view.y ? lines - view.y : 0;
    }

protected:
    // The lines to show, as markup the screen prints, wrapped to a width
    [[nodiscard]] virtual std::vector<std::string> Lines(const CScreen& screen, std::size_t width) = 0;

public:
    // A window that floats at first where it is given, and is never made
    // smaller than its minimum size, frame included; following the latest
    // line, or not
    CTextWindow(std::string title, const SRect& floating, Size min_size, bool a_follows = false,
                EWindowOptions options = EWindowOptions::None)
        : CWindow(std::move(title), floating, min_size, options), follows(a_follows)
    {
    }

    void Draw(CScreen& screen, const SRect& inside) override
    {
        const bool latest = follows && first >= Last();
        const std::vector<std::string> shown = Lines(screen, inside.size.x);
        lines = shown.size();
        view = inside.size;
        first = latest ? Last() : std::min(first, Last());

        for (std::size_t row = 0; row < view.y && first + row < lines; ++row) {
            screen.Print(inside.corner.x, inside.corner.y + row, shown[first + row]);
        }
    }

    void Scroll(int by) override
    {
        const std::size_t steps = static_cast<std::size_t>(by < 0 ? -static_cast<long long>(by) : by);
        setScrollOffset(Position(0, by < 0 ? first - std::min(first, steps) : first + steps));
    }

    [[nodiscard]] std::optional<SScrollInfo> getScrollInfo() const override
    {
        return SScrollInfo{Size(view.x, lines), view, Position(0, first)};
    }

    [[nodiscard]] bool supportsScrollDragging() const override
    {
        return true;
    }

    void setScrollOffset(Position offset) override
    {
        first = std::min(offset.y, Last());
    }

    // Back to the first line, as when the window shows something else
    void ScrollToTop()
    {
        first = 0;
    }

    // Lines scrolled down from the first, and back from the last
    [[nodiscard]] std::size_t getScrolled() const
    {
        return first;
    }

    [[nodiscard]] std::size_t getScrolledBack() const
    {
        return Last() - std::min(first, Last());
    }
};

} // namespace RL

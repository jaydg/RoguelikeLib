// Tests for RTUI's rtui.windows: windows laid out, drawn, and moved,
// resized, docked, tabbed and closed with the mouse

import rtui.screen;
import rtui.windows;
import rltests.check;
import std;

using RL::tests::Check;
using RL::tests::Holds;
using RL::tests::Row;

namespace
{

// A window that fills itself with a letter, and remembers what was done
// to it
class CTestWindow : public RL::CWindow
{
public:
    char32_t fill;
    RL::SRect drawn;
    int scrolled = 0;
    std::optional<RL::Position> clicked;

    CTestWindow(std::string title, const RL::SRect& floating, char32_t a_fill)
        : RL::CWindow(std::move(title), floating, RL::Size(10, 4)), fill(a_fill)
    {
    }

    void Draw(RL::CScreen& screen, const RL::SRect& inside) override
    {
        drawn = inside;

        // More than fits, to see that it is clipped
        for (std::size_t y = 0; y < screen.getSize().y; ++y) {
            for (std::size_t x = 0; x < screen.getSize().x; ++x) {
                screen.Put(x, y, fill, 0x123456);
            }
        }
    }

    void Scroll(int lines) override
    {
        scrolled += lines;
    }

    void Click(RL::Position at, RL::EMouseButton) override
    {
        clicked = at;
    }
};

RL::SMouse Mouse(RL::EMouseAction action, std::size_t x, std::size_t y)
{
    RL::SMouse mouse;
    mouse.action = action;
    mouse.button = action == RL::EMouseAction::ScrollUp || action == RL::EMouseAction::ScrollDown ?
                   RL::EMouseButton::None : RL::EMouseButton::Left;
    mouse.position = RL::Position(x, y);
    return mouse;
}

// Drags with the left button from one cell to another
void DragFromTo(RL::CWindowManager& manager, std::size_t from_x, std::size_t from_y, std::size_t to_x, std::size_t to_y)
{
    manager.HandleMouse(Mouse(RL::EMouseAction::Press, from_x, from_y));
    manager.HandleMouse(Mouse(RL::EMouseAction::Drag, to_x, to_y));
    manager.HandleMouse(Mouse(RL::EMouseAction::Release, to_x, to_y));
}

RL::SRect Rect(std::size_t x, std::size_t y, std::size_t width, std::size_t height)
{
    return {RL::Position(x, y), RL::Size(width, height)};
}

} // namespace

void TestRectangles()
{
    constexpr RL::SRect crect{RL::Position(2, 3), RL::Size(4, 5)};
    static_assert(crect.Right() == 6);
    static_assert(crect.Bottom() == 8);
    static_assert(crect.Contains(RL::Position(2, 3)));
    static_assert(crect.Contains(2, 3));
    static_assert(crect.Contains(5, 7));
    static_assert(!crect.Contains(6, 7));
    static_assert(!crect.Contains(1, 3));
    static_assert(crect.Intersection(RL::SRect{RL::Position(4, 0), RL::Size(10, 4)}) == RL::SRect{RL::Position(4, 3), RL::Size(2, 1)});
    static_assert(crect.Intersection(RL::SRect{RL::Position(10, 10), RL::Size(1, 1)}).Empty());
    static_assert(crect.Inset(1) == RL::SRect{RL::Position(3, 4), RL::Size(2, 3)});
    static_assert(crect.Inset(2).Empty());
    static_assert(noexcept(crect.Right()));
    static_assert(noexcept(crect.Contains(2, 3)));
    static_assert(noexcept(crect.Intersection(crect)));

    const RL::SRect rect = Rect(2, 3, 4, 5);
    Check(rect.Right() == 6 && rect.Bottom() == 8 && rect.Contains(RL::Position(2, 3)) && rect.Contains(2, 3) &&
          rect.Contains(RL::Position(5, 7)) && rect.Contains(5, 7) &&
          !rect.Contains(RL::Position(6, 7)) && !rect.Contains(6, 7) &&
          !rect.Contains(RL::Position(1, 3)) && !rect.Contains(1, 3), "a rectangle and what it contains");
    Check(rect.Intersection(Rect(4, 0, 10, 4)) == Rect(4, 3, 2, 1) && rect.Intersection(Rect(10, 10, 1, 1)).Empty(),
          "what two rectangles have in common");
    Check(rect.Inset(1) == Rect(3, 4, 2, 3) && rect.Inset(2).Empty(), "a rectangle made smaller on every side");

    RL::CScreen screen(RL::Size(6, 2));
    screen.setClip(Rect(1, 0, 3, 1));
    screen.Print(0, 0, "abcdef");
    screen.Put(1, 1, U'x', 0);
    Check(Row(screen, 0) == U" bcd  " && Row(screen, 1) == U"      ", "drawing is clipped");
    screen.resetClip();
    screen.Put(1, 1, U'x', 0);
    Check(Row(screen, 1) == U" x    ", "until the clip is reset");
}

void TestDoubleClick()
{
    using RL::EDock;
    using RL::EMouseAction;
    using namespace std::chrono_literals;

    RL::CWindowManager manager;
    CTestWindow window("Window", Rect(10, 5, 20, 8), U'w');
    const RL::SRect area = Rect(0, 1, 80, 22);
    manager.Open(window);
    manager.Layout(area);

    const auto start = std::chrono::steady_clock::time_point{};
    const auto click = [&](std::size_t x, std::size_t y, std::chrono::milliseconds at) {
        manager.HandleMouse(Mouse(EMouseAction::Press, x, y), start + at);
        manager.HandleMouse(Mouse(EMouseAction::Release, x, y), start + at + 50ms);
        manager.Layout(area);
    };

    click(15, 5, 0ms);
    click(15, 5, 1000ms);
    Check(window.getFrame() == Rect(10, 5, 20, 8), "two clicks on a title far apart are only clicks");

    click(15, 5, 1200ms);
    Check(window.getFrame() == area, "a double-click on the title makes a window fill the area");

    click(15, 1, 2000ms);
    click(15, 1, 2200ms);
    Check(window.getFrame() == Rect(10, 5, 20, 8), "another makes it as small as it was");

    // As wide as the area, from dragging its edge, it is halved
    DragFromTo(manager, 15, 5, 5, 5);
    manager.Layout(area);
    DragFromTo(manager, 19, 8, 79, 8);
    manager.Layout(area);
    Check(window.getFrame() == Rect(0, 5, 80, 8), "dragged as wide as the area");
    click(15, 5, 3000ms);
    click(15, 5, 3200ms);
    Check(window.getFrame() == Rect(20, 5, 40, 8), "a double-click halves what fills the area");

    // Without the time, there are no double-clicks
    manager.HandleMouse(Mouse(EMouseAction::Press, 25, 5));
    manager.HandleMouse(Mouse(EMouseAction::Release, 25, 5));
    manager.HandleMouse(Mouse(EMouseAction::Press, 25, 5));
    manager.HandleMouse(Mouse(EMouseAction::Release, 25, 5));
    manager.Layout(area);
    Check(window.getFrame() == Rect(20, 5, 40, 8), "without the time of the clicks, there are none");

    // Docked, it takes the whole area across, or half of it
    manager.Dock(window, EDock::Left, 30);
    manager.Layout(area);
    click(3, 1, 4000ms);
    click(3, 1, 4200ms);
    Check(window.getDockSize() == 80, "docked, a double-click on the title makes it take the whole area");
    click(3, 1, 5000ms);
    click(3, 1, 5200ms);
    Check(window.getDockSize() == 30, "and another as large as it was");

    manager.Dock(window, EDock::Left, 80);
    manager.Layout(area);
    click(3, 1, 6000ms);
    click(3, 1, 6200ms);
    Check(window.getDockSize() == 40, "a docked window taking it all is halved");

    manager.Close(window);
}

void TestWindowManager()
{
    using RL::EDock;
    using RL::EMouseAction;

    RL::CWindowManager manager;
    CTestWindow log("Log", Rect(0, 0, 20, 6), U'l');
    CTestWindow side("Side", Rect(0, 0, 20, 6), U's');
    CTestWindow first("First", Rect(10, 5, 20, 8), U'1');
    CTestWindow second("Second", Rect(15, 7, 20, 8), U'2');

    // Docked windows take room from the edges, the first docked outermost
    const RL::SRect area = Rect(0, 1, 80, 22);
    manager.Dock(log, EDock::Bottom, 7);
    manager.Dock(side, EDock::Right, 20);
    manager.Open(log);
    manager.Open(side);
    manager.Open(first);
    manager.Open(second);

    const RL::SRect free = manager.Layout(area);
    Check(log.getFrame() == Rect(0, 16, 80, 7), "a window docked to the bottom takes the bottom rows");
    Check(side.getFrame() == Rect(60, 1, 20, 15), "the next docked takes from what is left");
    Check(free == Rect(0, 1, 60, 15), "the rest is left free");
    Check(first.getFrame() == Rect(10, 5, 20, 8) && manager.Top() == &second, "floating windows float, the last opened on top");

    // Drawn over the screen: framed and titled, the content clipped to
    // the inside
    RL::CScreen screen(RL::Size(80, 24));
    manager.Draw(screen);
    Check(screen.At(10, 5).glyph == U'┌' && screen.At(29, 5).glyph == U'┐' && screen.At(10, 12).glyph == U'└' &&
          Holds(Row(screen, 5), U" First ") && screen.At(28, 5).glyph == U'×', "a window is framed and titled");
    Check(screen.At(34, 14).glyph == U'◢' && screen.At(79, 22).glyph == U'┘', "floating windows can be resized at the corner");
    Check(first.drawn == Rect(11, 6, 18, 6) && screen.At(11, 6).glyph == U'1', "the content is drawn inside");
    Check(screen.At(15, 7).glyph == U'┌' && screen.At(16, 8).glyph == U'2' && screen.At(0, 0).glyph == U' ',
          "the window on top covers those below, and nothing goes outside");

    // The wheel and clicks go to the window under the mouse, on top
    Check(manager.HandleMouse(Mouse(EMouseAction::ScrollUp, 12, 7)) && first.scrolled == -RL::scroll_lines,
          "the wheel scrolls the window under it");
    Check(manager.HandleMouse(Mouse(EMouseAction::Press, 20, 10)) && second.clicked == RL::Position(4, 2) &&
          !first.clicked, "a click goes to the window on top, inside it");
    Check(!manager.HandleMouse(Mouse(EMouseAction::Press, 2, 2)), "a click outside the windows is not theirs");

    // Clicking a window brings it to the top
    manager.HandleMouse(Mouse(EMouseAction::Press, 12, 7));
    manager.HandleMouse(Mouse(EMouseAction::Release, 12, 7));
    Check(manager.Top() == &first && first.clicked == RL::Position(1, 1), "a click brings a window to the top");

    // Dragging the title moves a window, as far as the area goes
    DragFromTo(manager, 15, 5, 25, 3);
    manager.Layout(area);
    Check(first.getFrame() == Rect(20, 3, 20, 8), "dragging the title moves a window");

    DragFromTo(manager, 25, 3, 78, 10);
    manager.Layout(area);
    Check(first.getFrame() == Rect(60, 10, 20, 8), "but not out of the area");

    DragFromTo(manager, 65, 10, 65, 2);
    manager.Layout(area);
    Check(first.getFrame() == Rect(60, 2, 20, 8) && first.getDock() == EDock::None, "short of the edge, it floats on");

    // The edges and corner resize it, never below its least size
    DragFromTo(manager, 79, 8, 69, 8);
    manager.Layout(area);
    Check(first.getFrame() == Rect(60, 2, 10, 8), "the right edge resizes it");

    DragFromTo(manager, 65, 9, 65, 3);
    manager.Layout(area);
    Check(first.getFrame() == Rect(60, 2, 10, 4), "the bottom edge resizes it, to no less than its least size");

    DragFromTo(manager, 69, 5, 74, 10);
    manager.Layout(area);
    Check(first.getFrame() == Rect(60, 2, 15, 9), "the corner resizes it both ways");

    // Clicking the title of a window at an edge does not dock it
    CTestWindow high("High", Rect(30, 1, 12, 5), U'h');
    manager.Open(high);
    manager.Layout(area);
    manager.HandleMouse(Mouse(EMouseAction::Press, 33, 1));
    manager.HandleMouse(Mouse(EMouseAction::Release, 33, 1));
    Check(high.getDock() == EDock::None && manager.Top() == &high, "a click on a title at the edge does not dock it");

    // Dragged to an edge, it does
    DragFromTo(manager, 33, 1, 33, 1);
    Check(high.getDock() == EDock::None, "nor does a drag that goes nowhere");
    manager.HandleMouse(Mouse(EMouseAction::Press, 33, 1));
    manager.HandleMouse(Mouse(EMouseAction::Drag, 34, 3));
    manager.HandleMouse(Mouse(EMouseAction::Drag, 34, 0));
    manager.HandleMouse(Mouse(EMouseAction::Release, 34, 0));
    Check(high.getDock() == EDock::Top && high.getDockSize() == 5, "dragged to the top, or past it, it docks there");
    manager.Close(high);

    // A docked window is resized by its inner edge, and floats again when
    // dragged by its title
    DragFromTo(manager, 40, 16, 40, 12);
    Check(log.getDockSize() == 11 && manager.Layout(area) == Rect(0, 1, 60, 11), "dragging a docked window's edge resizes it");

    log.drawn = {};
    DragFromTo(manager, 3, 12, 20, 5);
    const RL::SRect after = manager.Layout(area);
    Check(log.getDock() == EDock::None && log.getFrame() == Rect(17, 5, 20, 6) && after == Rect(0, 1, 60, 22),
          "dragged by its title, a docked window floats as large as it was");

    // Dragged to an edge, it docks there
    DragFromTo(manager, 20, 5, 20, 22);
    Check(log.getDock() == EDock::Bottom && log.getDockSize() == 6, "dragged to the bottom, a window docks there");
    Check(manager.Layout(area) == Rect(0, 1, 60, 16) && log.getFrame() == Rect(0, 17, 60, 6),
          "inside of the windows docked before");

    // The × closes a window
    Check(manager.IsOpen(second), "a window is open");
    manager.Layout(area);
    const RL::SRect frame = second.getFrame();
    manager.HandleMouse(Mouse(EMouseAction::Press, frame.Right() - 2, frame.corner.y));
    Check(!manager.IsOpen(second) && manager.IsOpen(first), "the × closes it");

    manager.Toggle(second);
    Check(manager.IsOpen(second) && manager.Top() == &second, "toggled, it opens again on top");
    manager.Toggle(second);
    Check(!manager.IsOpen(second), "and closes");

    // Without room, a docked window is not shown until there is
    manager.Dock(side, EDock::Right, 20);
    const RL::SRect cramped = manager.Layout(Rect(0, 0, 25, 5));
    Check(side.getFrame().Empty() && cramped.size.x == 25, "a window without room is not shown");

    // A layout taken from the windows and given back puts them where they
    // were, docked in the same order
    const RL::SWindowLayout log_layout = manager.getLayout(log);
    const RL::SWindowLayout side_layout = manager.getLayout(side);
    RL::CWindowManager again;
    CTestWindow log_again("Log", Rect(0, 0, 1, 1), U'l');
    CTestWindow side_again("Side", Rect(0, 0, 1, 1), U's');
    again.setLayout(side_again, side_layout);
    again.setLayout(log_again, log_layout);
    Check(again.getLayout(log_again) == log_layout && again.getLayout(side_again) == side_layout,
          "a layout is put back as it was");
    again.Layout(area);
    manager.Layout(area);
    Check(log_again.getFrame() == log.getFrame() && side_again.getFrame() == side.getFrame(),
          "and laid out the same, also if given back in another order");

    for (CTestWindow* window : {
                &log, &side, &first, &second
            }) {
        manager.Close(*window);
    }

    again.Close(log_again);
    again.Close(side_again);
}

void TestNarrowTabs()
{
    const auto strip = [](std::size_t width) {
        CTestWindow alpha("Alpha", Rect(0, 0, 10, 5), U'a');
        CTestWindow beta("Beta", Rect(0, 0, 10, 5), U'b');
        CTestWindow gamma("Gamma", Rect(0, 0, 10, 5), U'g');
        CTestWindow delta("Delta", Rect(0, 0, 10, 5), U'd');
        RL::CWindowManager manager;

        for (CTestWindow* window : {
                    &alpha, &beta, &gamma, &delta
                }) {
            manager.Dock(*window, RL::EDock::Left, width);
            manager.Tab(*window, alpha);
            manager.Open(*window);
        }

        RL::CScreen screen(RL::Size(60, 10));
        static_cast<void>(manager.Layout(Rect(0, 0, 60, 10)));
        manager.Draw(screen);
        const std::u32string row = Row(screen, 1).substr(0, width);

        for (CTestWindow* window : {
                    &alpha, &beta, &gamma, &delta
                }) {
            manager.Close(*window);
        }

        return row;
    };

    const std::u32string wide = strip(50);
    Check(Holds(wide, U" Alpha ") && Holds(wide, U" Delta "), "tabs that fit are shown whole");
    const std::u32string narrow = strip(30);
    Check(Holds(narrow, U"Alph") && Holds(narrow, U"Bet") && Holds(narrow, U"Gamm") && Holds(narrow, U"Delt"),
          "a narrow group shortens its tabs alike, showing all of them");
    const std::u32string narrower = strip(16);
    Check(Holds(narrower, U"┐┌") && Holds(narrower, U'A') && Holds(narrower, U'B') && Holds(narrower, U'G') &&
          Holds(narrower, U'D'), "narrower still, they lose the spaces between them, but all still show");
    const std::u32string narrowest = strip(10);
    Check(Holds(narrowest, U'A') && Holds(narrowest, U'B') && !Holds(narrowest, U'G'),
          "too narrow for a letter each, the last are left out");
}


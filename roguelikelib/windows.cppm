//////////////////////////////////////////////////////////////////////////
// Windows
//
// Windows drawn over a screen: framed, with a title, moved and resized
// with the mouse, and closed with the × in the top right corner. A window
// floats over everything else, or is docked to an edge, where it takes the
// room it needs from what is left for the rest - a log along the bottom of
// the screen, for instance, with the map above it.
//
// What a window shows is its own business: a window is a class derived
// from CWindow, which draws its content into the rectangle inside its
// frame. CWindowManager knows which windows are open, lays them out, draws
// their frames and handles the mouse:
//
// dragging the title bar          moves a floating window
// dragging the right or bottom
// edge, or the corner ◢           resizes it
// dragging it to an edge          docks it there
// dragging a docked window's
// inner edge                      resizes it
// dragging its title              floats it again
// double-clicking the title       makes it large, or small again
// clicking ×                      closes it
// the wheel                       scrolls it, if it scrolls
//
// The windows belong to the caller; the manager only refers to them, and
// a window has to be closed before it goes away.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.windows;

export import rl.mouse;
export import rl.position;
export import rl.screen;
import std;

export namespace RL
{

enum class EDock {
    None,
    Top,
    Bottom,
    Left,
    Right
};

struct SWindowStyle {
    std::uint32_t frame = 0x707070;
    std::uint32_t title = 0xA0A0A0;

    // The window on top, which the last click went to
    std::uint32_t focused_frame = 0xC0C0C0;
    std::uint32_t focused_title = 0xFFFFFF;
};

// Where a window is, and how it is shown, e.g. to keep it for next time
struct SWindowLayout {
    bool open = false;
    EDock dock = EDock::None;

    // Rows of a window docked to the top or bottom, columns of one docked
    // to the left or right, its frame included
    std::size_t dock_size = 0;

    // Of windows docked to the same edge, the one docked first is laid out
    // nearest to it
    std::uint64_t docked_at = 0;

    // Where it floats when it is not docked
    SRect floating;

    bool operator==(const SWindowLayout&) const = default;
};

// How many lines a turn of the wheel scrolls
constexpr int scroll_lines = 3;

class CWindow
{
private:
    friend class CWindowManager;

    std::string title;
    SRect floating;
    Size min_size;
    EDock dock = EDock::None;
    std::size_t dock_size = 0;

    // When it was docked: windows docked first are laid out nearest to
    // the edge
    std::uint64_t docked_at = 0;

    // Where it was laid out last; empty when there was no room for it
    SRect frame;

    // How large it was before a double-click on its title made it larger
    // or smaller, to go back to with the next: floating, or docked
    std::optional<SRect> restore_floating;
    std::optional<std::size_t> restore_dock_size;

public:
    // A window that floats at first where it is given, and is never made
    // smaller than its minimum size, frame included
    CWindow(std::string a_title, const SRect& a_floating, Size a_min_size = Size(12, 3))
        : title(std::move(a_title)), floating(a_floating),
          min_size(std::max<std::size_t>(a_min_size.x, 5), std::max<std::size_t>(a_min_size.y, 3))
    {
    }

    CWindow(const CWindow&) = delete;
    CWindow& operator=(const CWindow&) = delete;
    virtual ~CWindow() = default;

    // Draws what the window shows into `inside`, the window within its
    // frame, which has been cleared. Drawing is clipped to it.
    virtual void Draw(CScreen& screen, const SRect& inside) = 0;

    // The wheel was turned over the window: up, with lines below zero, or
    // down
    virtual void Scroll(int lines)
    {
        static_cast<void>(lines);
    }

    // A button was pressed inside, at a position from the corner of the
    // inside
    virtual void Click(Position at, EMouseButton button)
    {
        static_cast<void>(at);
        static_cast<void>(button);
    }

    [[nodiscard]] const std::string& getTitle() const
    {
        return title;
    }

    void setTitle(std::string a_title)
    {
        title = std::move(a_title);
    }

    // Where the window was laid out last, frame included
    [[nodiscard]] const SRect& getFrame() const
    {
        return frame;
    }

    [[nodiscard]] EDock getDock() const
    {
        return dock;
    }

    [[nodiscard]] std::size_t getDockSize() const
    {
        return dock_size;
    }

    [[nodiscard]] const SRect& getFloating() const
    {
        return floating;
    }

    [[nodiscard]] Size getMinSize() const
    {
        return min_size;
    }
};

class CWindowManager
{
private:
    // The open windows, the one on top last
    std::vector<CWindow*> windows;

    // Where windows go, as laid out last
    SRect area;

    SWindowStyle style;
    std::uint64_t next_docked_at = 1;

    enum class EGrab {
        Move,
        Undock,
        ResizeRight,
        ResizeBottom,
        ResizeCorner,
        ResizeDock
    };

    // A window being dragged, and where it was taken hold of, from the
    // corner of its frame
    struct SDrag {
        CWindow* window = nullptr;
        EGrab grab = EGrab::Move;
        Position offset{0, 0};

        // Where the button was pressed, and whether the mouse has left
        // that cell since: a window only clicked is not moved
        Position from{0, 0};
        bool moved = false;
    };

    std::optional<SDrag> drag;

    // Two clicks on a window's title within this time are a double-click
    static constexpr std::chrono::milliseconds double_click_time{400};

    // The last click on a window's title, to tell a double-click by
    struct STitleClick {
        CWindow* window = nullptr;
        Position at{0, 0};
        std::chrono::steady_clock::time_point time;
    };

    std::optional<STitleClick> title_click;

    static bool Vertical(EDock dock)
    {
        return dock == EDock::Top || dock == EDock::Bottom;
    }

    // Docked windows first, those docked first at the bottom; then the
    // floating ones, the one on top last
    [[nodiscard]] std::vector<CWindow*> InDrawingOrder() const
    {
        std::vector<CWindow*> order;

        for (CWindow* window : windows) {
            if (window->dock != EDock::None) {
                order.push_back(window);
            }
        }

        std::ranges::sort(order, {}, &CWindow::docked_at);

        for (CWindow* window : windows) {
            if (window->dock == EDock::None) {
                order.push_back(window);
            }
        }

        return order;
    }

    // The window on top at a position, if any
    [[nodiscard]] CWindow* At(Position pos) const
    {
        const std::vector<CWindow*> order = InDrawingOrder();

        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            if ((*it)->frame.Contains(pos)) {
                return *it;
            }
        }

        return nullptr;
    }

    // A floating window's rectangle moved and shrunk into the area as far
    // as it has to be
    [[nodiscard]] SRect Clamp(const SRect& rect, Size min_size) const
    {
        const auto axis = [](std::size_t at, std::size_t length, std::size_t least, std::size_t start, std::size_t room) {
            length = std::min(std::max(length, least), room);
            at = std::clamp(at, start, start + room - length);
            return std::pair(at, length);
        };

        if (area.Empty()) {
            return {area.corner, Size(0, 0)};
        }

        const auto[x, width] = axis(rect.corner.x, rect.size.x, min_size.x, area.corner.x, area.size.x);
        const auto[y, height] = axis(rect.corner.y, rect.size.y, min_size.y, area.corner.y, area.size.y);

        return {Position(x, y), Size(width, height)};
    }

    // What dragging from a position on a window's frame does, if anything
    [[nodiscard]] std::optional<EGrab> GrabAt(const CWindow& window, Position pos) const
    {
        const SRect& frame = window.frame;
        const bool top = pos.y == frame.corner.y;
        const bool bottom = pos.y + 1 == frame.Bottom();
        const bool left = pos.x == frame.corner.x;
        const bool right = pos.x + 1 == frame.Right();

        if (window.dock == EDock::None) {
            if (top) {
                return EGrab::Move;
            }

            if (bottom && right) {
                return EGrab::ResizeCorner;
            }

            if (right) {
                return EGrab::ResizeRight;
            }

            if (bottom) {
                return EGrab::ResizeBottom;
            }

            return std::nullopt;
        }

        // A docked window is taken by its title to float it again, and by
        // its inner edge to resize it
        if (top && pos.x > frame.corner.x && pos.x < frame.corner.x + 3 + window.title.size()) {
            return EGrab::Undock;
        }

        const bool inner = (window.dock == EDock::Bottom && top) || (window.dock == EDock::Top && bottom) ||
                           (window.dock == EDock::Left && right) || (window.dock == EDock::Right && left);

        return inner ? std::optional(EGrab::ResizeDock) : std::nullopt;
    }

    // Off the area, the mouse is taken to be at its edge
    [[nodiscard]] Position IntoArea(Position pos) const
    {
        return Position(std::clamp(pos.x, area.corner.x, area.Right() - 1), std::clamp(pos.y, area.corner.y, area.Bottom() - 1));
    }

    // The edge of the area a position is on, if any
    [[nodiscard]] EDock EdgeAt(Position pos) const
    {
        if (pos.y == area.corner.y) {
            return EDock::Top;
        }

        if (pos.y + 1 == area.Bottom()) {
            return EDock::Bottom;
        }

        if (pos.x == area.corner.x) {
            return EDock::Left;
        }

        if (pos.x + 1 == area.Right()) {
            return EDock::Right;
        }

        return EDock::None;
    }

    // A window is large when it fills the area across or down, floating;
    // docked, when it takes more than half of it. A double-click on its
    // title makes a large window small - as it was before, or half the area
    // - and a small one large, filling the area.
    void ToggleSize(CWindow& window)
    {
        if (window.dock == EDock::None) {
            const SRect& frame = window.frame;
            const bool wide = frame.size.x >= area.size.x;
            const bool tall = frame.size.y >= area.size.y;

            if (!wide && !tall) {
                window.restore_floating = window.floating;
                window.floating = area;
            } else if (window.restore_floating && window.restore_floating->size.x < area.size.x &&
                       window.restore_floating->size.y < area.size.y) {
                window.floating = *window.restore_floating;
                window.restore_floating.reset();
            } else {
                // What fills the area is halved, in the middle of it
                SRect smaller = frame;

                if (wide) {
                    smaller.size.x = area.size.x / 2;
                    smaller.corner.x = area.corner.x + (area.size.x - smaller.size.x) / 2;
                }

                if (tall) {
                    smaller.size.y = area.size.y / 2;
                    smaller.corner.y = area.corner.y + (area.size.y - smaller.size.y) / 2;
                }

                window.floating = smaller;
                window.restore_floating.reset();
            }

            window.floating = Clamp(window.floating, window.min_size);
            return;
        }

        const bool vertical = Vertical(window.dock);
        const std::size_t room = vertical ? area.size.y : area.size.x;
        const std::size_t least = vertical ? window.min_size.y : window.min_size.x;
        const std::size_t half = std::max(room / 2, least);
        const std::size_t size = std::min(window.dock_size, room);

        if (size > half) {
            const bool back = window.restore_dock_size && *window.restore_dock_size <= half;
            window.dock_size = back ? std::max(*window.restore_dock_size, least) : half;
            window.restore_dock_size = back ? std::nullopt : std::optional(size);
        } else {
            window.restore_dock_size = size;
            window.dock_size = room;
        }
    }

    // Whether a press on a window's title at a moment makes a double-click
    // with the one before it; it is remembered for the next if not
    bool DoubleClicked(CWindow& window, Position pos, std::optional<std::chrono::steady_clock::time_point> now)
    {
        if (!now) {
            return false;
        }

        const bool twice = title_click && title_click->window == &window && title_click->at == pos &&
                           *now - title_click->time <= double_click_time;

        if (twice) {
            title_click.reset();
        } else {
            title_click = STitleClick{&window, pos, * now};
        }

        return twice;
    }

    void Press(CWindow& window, const SMouse& mouse, std::optional<std::chrono::steady_clock::time_point> now)
    {
        Raise(window);

        const SRect& frame = window.frame;
        const Position pos = mouse.position;

        if (mouse.button != EMouseButton::Left) {
            if (const SRect inside = frame.Inset(1); inside.Contains(pos)) {
                window.Click(Position(pos.x - inside.corner.x, pos.y - inside.corner.y), mouse.button);
            }

            return;
        }

        if (pos.y == frame.corner.y && pos.x + 2 == frame.Right()) {
            Close(window);
            return;
        }

        if (const auto grab = GrabAt(window, pos)) {
            if ((grab == EGrab::Move || grab == EGrab::Undock) && DoubleClicked(window, pos, now)) {
                ToggleSize(window);
                return;
            }

            drag = SDrag{&window, * grab, Position(pos.x - frame.corner.x, pos.y - frame.corner.y), pos};
            return;
        }

        if (const SRect inside = frame.Inset(1); inside.Contains(pos)) {
            window.Click(Position(pos.x - inside.corner.x, pos.y - inside.corner.y), mouse.button);
        }
    }

    void Drag(Position pos)
    {
        CWindow& window = *drag->window;
        SRect& floating = window.floating;

        drag->moved = drag->moved || pos != drag->from;

        if (!drag->moved) {
            return;
        }

        // A window dragged was not clicked
        title_click.reset();


        switch (drag->grab) {
        case EGrab::Undock:
            // It floats as large as it was the last time it did, with the
            // title still under the mouse
            window.dock = EDock::None;

            if (floating.Empty()) {
                floating.size = Size(std::max<std::size_t>(window.min_size.x, 30), std::max<std::size_t>(window.min_size.y, 8));
            }

            drag->offset = Position(std::min(drag->offset.x, floating.size.x - 1), 0);
            drag->grab = EGrab::Move;
            [[fallthrough]];

        case EGrab::Move:
            floating.corner = Position(pos.x - std::min(drag->offset.x, pos.x), pos.y - std::min(drag->offset.y, pos.y));
            floating = Clamp(floating, window.min_size);
            break;

        case EGrab::ResizeRight:
        case EGrab::ResizeBottom:
        case EGrab::ResizeCorner:
            floating = window.frame;

            if (drag->grab != EGrab::ResizeBottom) {
                floating.size.x = std::max(pos.x + 1, floating.corner.x + window.min_size.x) - floating.corner.x;
            }

            if (drag->grab != EGrab::ResizeRight) {
                floating.size.y = std::max(pos.y + 1, floating.corner.y + window.min_size.y) - floating.corner.y;
            }

            floating = Clamp(floating, window.min_size);
            break;

        case EGrab::ResizeDock: {
            const SRect& frame = window.frame;
            std::size_t size = 0;

            switch (window.dock) {
            case EDock::Bottom:
                size = frame.Bottom() - std::min(pos.y, frame.Bottom() - 1);
                break;

            case EDock::Top:
                size = std::max(pos.y, frame.corner.y) - frame.corner.y + 1;
                break;

            case EDock::Right:
                size = frame.Right() - std::min(pos.x, frame.Right() - 1);
                break;

            case EDock::Left:
                size = std::max(pos.x, frame.corner.x) - frame.corner.x + 1;
                break;

            case EDock::None:
                break;
            }

            window.dock_size = std::max(size, Vertical(window.dock) ? window.min_size.y : window.min_size.x);
            break;
        }
        }
    }

    // A window dragged by its title to an edge of the area docks there
    void Release(Position pos)
    {
        CWindow& window = *drag->window;
        const EDock edge = EdgeAt(pos);

        if (drag->grab == EGrab::Move && drag->moved && edge != EDock::None) {
            Dock(window, edge, Vertical(edge) ? window.floating.size.y : window.floating.size.x);
        }

        drag.reset();
    }

    void DrawFrame(CScreen& screen, const CWindow& window, bool focused) const
    {
        const SRect& frame = window.frame;
        const std::uint32_t colour = focused ? style.focused_frame : style.frame;
        const std::size_t right = frame.Right() - 1;
        const std::size_t bottom = frame.Bottom() - 1;

        for (std::size_t x = frame.corner.x + 1; x < right; ++x) {
            screen.Put(x, frame.corner.y, U'─', colour);
            screen.Put(x, bottom, U'─', colour);
        }

        for (std::size_t y = frame.corner.y + 1; y < bottom; ++y) {
            screen.Put(frame.corner.x, y, U'│', colour);
            screen.Put(right, y, U'│', colour);
        }

        screen.Put(frame.corner.x, frame.corner.y, U'┌', colour);
        screen.Put(right, frame.corner.y, U'┐', colour);
        screen.Put(frame.corner.x, bottom, U'└', colour);
        screen.Put(right, bottom, window.dock == EDock::None ? U'◢' : U'┘', colour);

        // The title, cut short before the ×
        screen.Put(right - 1, frame.corner.y, U'×', colour);
        screen.setClip({Position(frame.corner.x + 1, frame.corner.y), Size(frame.size.x - 3, 1)});
        screen.Print(frame.corner.x + 1, frame.corner.y, " " + window.title + " ", focused ? style.focused_title : style.title);
        screen.resetClip();
    }

public:
    void setStyle(const SWindowStyle& a_style)
    {
        style = a_style;
    }

    // Opens a window on top of the others, or brings it to the top if it
    // is open already
    void Open(CWindow& window)
    {
        Raise(window);
    }

    void Close(CWindow& window)
    {
        std::erase(windows, &window);

        if (drag && drag->window == &window) {
            drag.reset();
        }

        if (title_click && title_click->window == &window) {
            title_click.reset();
        }
    }

    void Toggle(CWindow& window)
    {
        if (IsOpen(window)) {
            Close(window);
        } else {
            Open(window);
        }
    }

    [[nodiscard]] bool IsOpen(const CWindow& window) const
    {
        return std::ranges::find(windows, &window) != windows.end();
    }

    // Brings a window to the top, opening it if it is closed
    void Raise(CWindow& window)
    {
        std::erase(windows, &window);
        windows.push_back(&window);
    }

    // The window on top, if any is open
    [[nodiscard]] CWindow* Top() const
    {
        return windows.empty() ? nullptr : windows.back();
    }

    // Docks a window to an edge, as the innermost of those docked there.
    // `size` is its rows docked to the top or bottom, its columns to the
    // left or right, frame included. EDock::None floats it again.
    void Dock(CWindow& window, EDock dock, std::size_t size)
    {
        window.dock = dock;
        window.dock_size = std::max(size, Vertical(dock) ? window.min_size.y : window.min_size.x);
        window.docked_at = next_docked_at++;
    }

    // Where a window is and how it is shown
    [[nodiscard]] SWindowLayout getLayout(const CWindow& window) const
    {
        return {IsOpen(window), window.dock, window.dock_size, window.docked_at, window.floating};
    }

    // Puts a window where a layout says, opening or closing it
    void setLayout(CWindow& window, const SWindowLayout& layout)
    {
        window.floating = layout.floating;
        window.dock = layout.dock;
        window.dock_size = layout.dock == EDock::None ? layout.dock_size :
                           std::max(layout.dock_size, Vertical(layout.dock) ? window.min_size.y : window.min_size.x);
        window.docked_at = layout.docked_at;
        next_docked_at = std::max(next_docked_at, layout.docked_at + 1);

        if (layout.open) {
            Open(window);
        } else {
            Close(window);
        }
    }

    // Lays the open windows out in an area of the screen: the docked ones
    // along its edges, the floating ones within it. Returns what is left
    // between the docked ones.
    SRect Layout(const SRect& a_area)
    {
        area = a_area;
        SRect free = area;

        for (CWindow* window : InDrawingOrder()) {
            if (window->dock == EDock::None) {
                window->frame = Clamp(window->floating, window->min_size);
                continue;
            }

            const bool vertical = Vertical(window->dock);
            const std::size_t room = vertical ? free.size.y : free.size.x;
            const std::size_t across = vertical ? free.size.x : free.size.y;
            const std::size_t least = vertical ? window->min_size.y : window->min_size.x;
            const std::size_t least_across = vertical ? window->min_size.x : window->min_size.y;

            // No room for it: it is not shown until there is
            if (room < least || across < least_across) {
                window->frame = {free.corner, Size(0, 0)};
                continue;
            }

            const std::size_t size = std::min(window->dock_size, room);

            switch (window->dock) {
            case EDock::Top:
                window->frame = {free.corner, Size(free.size.x, size)};
                free.corner.y += size;
                free.size.y -= size;
                break;

            case EDock::Bottom:
                window->frame = {Position(free.corner.x, free.Bottom() - size), Size(free.size.x, size)};
                free.size.y -= size;
                break;

            case EDock::Left:
                window->frame = {free.corner, Size(size, free.size.y)};
                free.corner.x += size;
                free.size.x -= size;
                break;

            case EDock::Right:
                window->frame = {Position(free.Right() - size, free.corner.y), Size(size, free.size.y)};
                free.size.x -= size;
                break;

            case EDock::None:
                break;
            }
        }

        return free;
    }

    // Draws the open windows as laid out last, over what is on the screen
    void Draw(CScreen& screen) const
    {
        for (CWindow* window : InDrawingOrder()) {
            const SRect& frame = window->frame;

            if (frame.size.x < 5 || frame.size.y < 3) {
                continue;
            }

            const SRect inside = frame.Inset(1);

            for (std::size_t y = inside.corner.y; y < inside.Bottom(); ++y) {
                for (std::size_t x = inside.corner.x; x < inside.Right(); ++x) {
                    screen.Put(x, y, U' ', default_text_colour);
                }
            }

            DrawFrame(screen, *window, window == Top());

            screen.setClip(inside);
            window->Draw(screen, inside);
            screen.resetClip();
        }
    }

    // Moves, resizes, docks, closes and scrolls windows as the mouse says.
    // Returns whether it was meant for a window, rather than for what is
    // under them. Told when the mouse did it, two clicks on a title in
    // quick succession are a double-click; without, there are none.
    bool HandleMouse(const SMouse& mouse, std::optional<std::chrono::steady_clock::time_point> now = std::nullopt)
    {
        const Position pos = mouse.position;

        if (drag) {
            if (area.Empty()) {
                drag.reset();
            } else if (mouse.action == EMouseAction::Drag) {
                Drag(IntoArea(pos));
            } else if (mouse.action == EMouseAction::Release) {
                Release(IntoArea(pos));
            }

            return true;
        }

        CWindow* window = At(pos);

        if (window == nullptr) {
            return false;
        }

        switch (mouse.action) {
        case EMouseAction::Press:
            Press(*window, mouse, now);
            break;

        case EMouseAction::ScrollUp:
            window->Scroll(-scroll_lines);
            break;

        case EMouseAction::ScrollDown:
            window->Scroll(scroll_lines);
            break;

        case EMouseAction::Release:
        case EMouseAction::Drag:
        case EMouseAction::Move:
            break;
        }

        return true;
    }
};

} // namespace RL

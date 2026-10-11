//////////////////////////////////////////////////////////////////////////
// Windows
//
// Windows drawn over a screen: framed, with a title, moved and resized
// with the mouse, and closed with the × in the top right corner. A window
// floats over everything else, or is docked to an edge, where it takes the
// room it needs from what is left for the rest - a log along the bottom of
// the screen, for instance, with the map above it.
//
// Windows docked to the same edge that are put in one tab group share the
// room they take: one frame, with the tabs of the group under its title,
// of which one window shows at a time.
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
// clicking a tab                  shows its window, in place of the
// others of its tab group
// the wheel                       scrolls it, if it scrolls
//
// The windows belong to the caller; the manager only refers to them, and
// a window has to be closed before it goes away.
//////////////////////////////////////////////////////////////////////////

module;

export module rtui.windows;

export import rtui.mouse;
export import rl.position;
export import rtui.screen;
export import rtui.terminal;
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

// Restrictions can be combined with bitwise OR. None keeps the default
// movable, resizable and closable behavior.
enum class EWindowOptions : unsigned {
    None = 0,
    NoResize = 1 << 0,
    NoMove = 1 << 1,
    NoClose = 1 << 2,
    NoUndock = 1 << 3
};

constexpr EWindowOptions operator|(EWindowOptions a, EWindowOptions b)
{
    return static_cast<EWindowOptions>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr EWindowOptions operator&(EWindowOptions a, EWindowOptions b)
{
    return static_cast<EWindowOptions>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}

constexpr EWindowOptions& operator|=(EWindowOptions& a, EWindowOptions b)
{
    a = a | b;
    return a;
}

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

    // The tab group it is in: docked to the same edge, windows of one
    // group share a frame and show as tabs of it, 0 for none
    std::uint64_t tab_group = 0;

    // Zero spans the available width (top/bottom) or height (left/right).
    std::size_t dock_cross_size = 0;

    bool operator==(const SWindowLayout&) const = default;
};

// How many lines a turn of the wheel scrolls
constexpr int scroll_lines = 3;

// Dimensions in cells and the origin of the visible content.
struct SScrollInfo {
    Size content{0, 0};
    Size viewport{0, 0};
    Position offset{0, 0};
};

class CWindow
{
private:
    friend class CWindowManager;

    std::string title;
    std::optional<std::string> frame_title;
    SRect floating;
    Size min_size;
    EWindowOptions options;
    EDock dock = EDock::None;
    std::size_t dock_size = 0;
    std::size_t dock_cross_size = 0;

    // When it was docked: windows docked first are laid out nearest to
    // the edge
    std::uint64_t docked_at = 0;

    // The tab group it is in, 0 for none: docked to the same edge,
    // windows of one group share a frame, one at a time behind tabs
    std::uint64_t tab_group = 0;

    // Where it was laid out last; empty when there was no room for it
    SRect frame;

    // How large it was before a double-click on its title made it larger
    // or smaller, to go back to with the next: floating, or docked
    std::optional<SRect> restore_floating;
    std::optional<std::size_t> restore_dock_size;

public:
    // A window that floats at first where it is given, and is never made
    // smaller than its minimum size, frame included
    CWindow(std::string a_title, const SRect& a_floating, Size a_min_size = Size(12, 3),
            EWindowOptions a_options = EWindowOptions::None)
        : title(std::move(a_title)), floating(a_floating),
          min_size(std::max<std::size_t>(a_min_size.x, 5), std::max<std::size_t>(a_min_size.y, 3)), options(a_options)
    {
    }

    CWindow(std::string title, const SRect& floating, EWindowOptions options, Size min_size = Size(12, 3))
        : CWindow(std::move(title), floating, min_size, options)
    {
    }

    [[nodiscard]] bool hasOption(EWindowOptions option) const
    {
        return (options & option) != EWindowOptions::None;
    }

    [[nodiscard]] EWindowOptions getOptions() const
    {
        return options;
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

    // Keyboard input for the focused window. Return true when consumed;
    // false lets the application handle shortcuts or other unhandled keys.
    virtual bool HandleKey(const SKey& key)
    {
        static_cast<void>(key);
        return false;
    }

    // Called after Draw() has laid out the content. No value means no
    // indicators. Indicators show position; input stays with the window.
    [[nodiscard]] virtual std::optional<SScrollInfo> getScrollInfo() const
    {
        return std::nullopt;
    }

    [[nodiscard]] virtual bool supportsScrollDragging() const
    {
        return false;
    }

    // Called for thumb dragging and page clicks. The manager clamps the
    // requested offset to the reported content and viewport dimensions.
    virtual void setScrollOffset(Position offset)
    {
        static_cast<void>(offset);
    }

    [[nodiscard]] const std::string& getTitle() const
    {
        return title;
    }

    void setTitle(std::string a_title)
    {
        title = std::move(a_title);
    }

    // An optional frame heading independent of the window's tab label.
    void setFrameTitle(std::string heading)
    {
        frame_title = std::move(heading);
    }

    [[nodiscard]] const std::string& getFrameTitle() const
    {
        return frame_title ? *frame_title : title;
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

    // The tab group the window shares a frame with when docked to the
    // same edge as the others of it, 0 for none
    [[nodiscard]] std::uint64_t getTabGroup() const
    {
        return tab_group;
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
    bool allow_tab_undocking = true;
    bool allow_tab_closing = true;
    std::uint64_t next_docked_at = 1;
    std::uint64_t next_tab_group = 1;

    enum class EGrab {
        Move,
        Undock,
        ResizeRight,
        ResizeBottom,
        ResizeCorner,
        ResizeDock,
        ResizeDockCross
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

    struct SScrollDrag {
        CWindow* window;
        bool vertical;
        std::size_t grab;
    };

    std::optional<SScrollDrag> scroll_drag;

    struct SScrollAxis {
        std::size_t length;
        std::size_t thumb;
        std::size_t start;
        std::size_t maximum;
    };

    static SScrollAxis ScrollAxis(std::size_t length, std::size_t content, std::size_t viewport, std::size_t offset)
    {
        if (length == 0 || viewport == 0 || content <= viewport) {
            return {length, 0, 0, 0};
        }

        const std::size_t thumb = std::clamp(static_cast<std::size_t>(
                                                 static_cast<long double>(viewport) / content * length), std::size_t{1}, length);
        const std::size_t maximum = content - viewport;
        const std::size_t start = static_cast<std::size_t>(
                                      static_cast<long double>(std::min(offset, maximum)) / maximum * (length - thumb));
        return {length, thumb, start, maximum};
    }

    bool PressScroll(CWindow& window, const SMouse& mouse)
    {
        const auto info = window.getScrollInfo();
        const SRect inside = ContentRect(window);

        if (mouse.button != EMouseButton::Left || !window.supportsScrollDragging() || !info || inside.Empty()) {
            return false;
        }

        const bool vertical = mouse.position.x == window.frame.Right() - 1 &&
                              mouse.position.y >= inside.corner.y && mouse.position.y < inside.Bottom();
        const bool horizontal = mouse.position.y == window.frame.Bottom() - 1 &&
                                mouse.position.x >= inside.corner.x && mouse.position.x < inside.Right();

        if (!vertical && !horizontal) {
            return false;
        }

        const auto axis = ScrollAxis(vertical ? inside.size.y : inside.size.x,
                                     vertical ? info->content.y : info->content.x,
                                     vertical ? info->viewport.y : info->viewport.x,
                                     vertical ? info->offset.y : info->offset.x);

        if (axis.thumb == 0) {
            return false;
        }

        Raise(window);
        title_click.reset();
        const std::size_t at = vertical ? mouse.position.y - inside.corner.y : mouse.position.x - inside.corner.x;

        if (at >= axis.start && at - axis.start < axis.thumb) {
            scroll_drag = SScrollDrag{&window, vertical, at - axis.start};
        } else {
            Position offset = info->offset;
            std::size_t& value = vertical ? offset.y : offset.x;
            value = std::min(value, axis.maximum);
            const std::size_t page = vertical ? info->viewport.y : info->viewport.x;
            value = at < axis.start ? value - std::min(page, value) : value + std::min(page, axis.maximum - value);
            window.setScrollOffset(offset);
        }

        return true;
    }

    void DragScroll(Position pos)
    {
        CWindow& window = *scroll_drag->window;
        const auto info = window.getScrollInfo();
        const SRect inside = ContentRect(window);
        const bool vertical = scroll_drag->vertical;

        if (!info || inside.Empty() || !window.supportsScrollDragging()) {
            scroll_drag.reset();
            return;
        }

        const auto axis = ScrollAxis(vertical ? inside.size.y : inside.size.x,
                                     vertical ? info->content.y : info->content.x,
                                     vertical ? info->viewport.y : info->viewport.x,
                                     vertical ? info->offset.y : info->offset.x);

        if (axis.thumb == 0 || axis.length == axis.thumb) {
            return;
        }

        const std::size_t coordinate = vertical ? pos.y : pos.x;
        const std::size_t corner = vertical ? inside.corner.y : inside.corner.x;
        const std::size_t relative = coordinate > corner ? coordinate - corner : 0;
        const std::size_t start = std::min(relative - std::min(relative, scroll_drag->grab), axis.length - axis.thumb);
        Position offset = info->offset;
        (vertical ? offset.y : offset.x) = static_cast<std::size_t>(std::round(
                                                                        static_cast<long double>(start) / (axis.length - axis.thumb) * axis.maximum));
        window.setScrollOffset(offset);
    }


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

    // The open windows in the tab group of `window`, docked to its edge
    // and ordered as they were docked. Fewer than two of them, and it
    // shares its frame with nobody - not even with those of its group
    // docked to another edge
    [[nodiscard]] std::vector<CWindow*> TabGroup(const CWindow& window) const
    {
        if (window.tab_group == 0 || window.dock == EDock::None) {
            return {};
        }

        std::vector<CWindow*> group;

        for (CWindow* candidate : windows) {
            if (candidate->tab_group == window.tab_group && candidate->dock == window.dock) {
                group.push_back(candidate);
            }
        }

        if (group.size() < 2) {
            return {};
        }

        std::ranges::stable_sort(group, {}, &CWindow::docked_at);
        return group;
    }

    // The tab of a group that shows: the one of them raised last. All of
    // a group is open, so one of them always is
    [[nodiscard]] CWindow* Active(const std::vector<CWindow*>& group) const
    {
        CWindow* active = nullptr;

        for (CWindow* window : windows) {
            if (std::ranges::find(group, window) != group.end()) {
                active = window;
            }
        }

        return active;
    }

    // Where the tabs of a group are drawn, in the strip under the title
    struct STab {
        CWindow* window = nullptr;
        std::size_t x = 0;     // column the box starts at
        std::size_t size = 0;  // cells the box takes, the ┌┐ included
    };

    // Where each tab of a group is drawn. A tab is its window's title in
    // a box, with a space after the frame before it and one between tabs;
    // one that does not fit whole is shortened, one that does not fit at
    // all is left out
    [[nodiscard]] std::vector<STab> TabsIn(const std::vector<CWindow*>& group, const SRect& frame) const
    {
        std::vector<STab> tabs;
        const std::size_t last = frame.Right() - 2;  // the last column the strip has
        std::size_t x = frame.corner.x + 2;          // the frame's side, then a space

        for (CWindow* window : group) {
            const std::size_t room = x <= last ? last - x + 1 : 0;
            const std::size_t size = std::min(window->title.size() + 4, room);

            if (size < 3) {
                break;
            }

            tabs.push_back({window, x, size});
            x += size + 1;
        }

        return tabs;
    }

    // Whether a frame has room for the tabs of its group under its title:
    // the strip, the rule under it and a line of content below. Too short
    // or too narrow, the window shows without them
    [[nodiscard]] bool HasTabs(const CWindow& window) const
    {
        return !TabGroup(window).empty() && window.frame.size.y >= 5 && window.frame.size.x >= 6;
    }

    // What is inside a window's frame for its content: below the title,
    // and below the tabs of a group when it shows them. Drawing into it
    // and clicks on it go to the window that shows
    [[nodiscard]] SRect ContentRect(const CWindow& window) const
    {
        const SRect& frame = window.frame;

        if (!HasTabs(window)) {
            return frame.Inset(1);
        }

        return {Position(frame.corner.x + 1, frame.corner.y + 3), Size(frame.size.x - 2, frame.size.y - 4)};
    }

    // The window on top at a position, if any
    [[nodiscard]] CWindow* At(Position pos) const
    {
        const std::vector<CWindow*> order = InDrawingOrder();

        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            if ((*it)->frame.Contains(pos)) {
                // A frame with tabs holds all of its group; anything on
                // it goes to the one of them that shows
                const auto group = TabGroup(**it);
                return group.empty() ? *it : Active(group);
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

        const bool resizable = !window.hasOption(EWindowOptions::NoResize);

        if (window.dock == EDock::None) {
            // Keep a resize target at the top-right corner too: the side
            // border may contain an interactive scrollbar.
            if (resizable && top && right) {
                return EGrab::ResizeRight;
            }

            if (top) {
                return window.hasOption(EWindowOptions::NoMove) ? std::nullopt : std::optional(EGrab::Move);
            }

            if (resizable && bottom && right) {
                return EGrab::ResizeCorner;
            }

            if (resizable && right) {
                return EGrab::ResizeRight;
            }

            if (resizable && bottom) {
                return EGrab::ResizeBottom;
            }

            return std::nullopt;
        }

        // A docked window may shrink across its edge without becoming
        // floating. Corners remain available when scrollbars occupy sides.
        if (resizable && ((Vertical(window.dock) && right) || (!Vertical(window.dock) && bottom))) {
            return EGrab::ResizeDockCross;
        }

        // A docked window is taken by its title to float it again, and by
        // its inner edge to resize it
        if (top && pos.x > frame.corner.x && pos.x < frame.corner.x + 3 + window.getFrameTitle().size()) {
            if (window.hasOption(EWindowOptions::NoUndock) || window.hasOption(EWindowOptions::NoMove) ||
                    (!allow_tab_undocking && window.tab_group != 0)) {
                return std::nullopt;
            }

            return EGrab::Undock;
        }

        const bool inner = (window.dock == EDock::Bottom && top) || (window.dock == EDock::Top && bottom) ||
                           (window.dock == EDock::Left && right) || (window.dock == EDock::Right && left);

        return inner && resizable ? std::optional(EGrab::ResizeDock) : std::nullopt;
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

    // How many rows or columns a docked window takes along its edge. The
    // others of its tab group take as many, as they share the slot with
    // it and switching tabs must not move the frame
    void SetDockSize(CWindow& window, std::size_t size)
    {
        window.dock_size = size;

        for (CWindow* member : TabGroup(window)) {
            member->dock_size = size;
        }
    }

    // A window is large when it fills the area across or down, floating;
    // docked, when it takes more than half of it. A double-click on its
    // title makes a large window small - as it was before, or half the area
    // - and a small one large, filling the area.
    void ToggleSize(CWindow& window)
    {
        if (window.hasOption(EWindowOptions::NoResize)) {
            return;
        }

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
            SetDockSize(window, back ? std::max(*window.restore_dock_size, least) : half);
            window.restore_dock_size = back ? std::nullopt : std::optional(size);
        } else {
            window.restore_dock_size = size;
            SetDockSize(window, room);
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
            if (const SRect inside = ContentRect(window); inside.Contains(pos)) {
                window.Click(Position(pos.x - inside.corner.x, pos.y - inside.corner.y), mouse.button);
            }

            return;
        }

        if (pos.y == frame.corner.y && pos.x + 2 == frame.Right()) {
            if (!window.hasOption(EWindowOptions::NoClose) && (allow_tab_closing || window.tab_group == 0)) {
                Close(window);
            }

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

        // The tabs of a group show in the two rows under the title: a
        // press on one of them shows that window, the rule under them is
        // nobody's content
        if (HasTabs(window)) {
            const std::size_t strip = frame.corner.y + 1;

            if (pos.y == strip) {
                const std::vector<STab> tabs = TabsIn(TabGroup(window), frame);

                for (const STab& tab : tabs) {
                    if (pos.x >= tab.x && pos.x < tab.x + tab.size) {
                        Raise(*tab.window);
                        break;
                    }
                }

                return;
            }

            if (pos.y == strip + 1) {
                return;
            }
        }

        if (const SRect inside = ContentRect(window); inside.Contains(pos)) {
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

        case EGrab::ResizeDockCross: {
            const bool vertical = Vertical(window.dock);
            const std::size_t at = vertical ? pos.x : pos.y;
            const std::size_t corner = vertical ? window.frame.corner.x : window.frame.corner.y;
            const std::size_t least = vertical ? window.min_size.x : window.min_size.y;
            const std::size_t size = std::max(at + 1, corner + least) - corner;
            window.dock_cross_size = size;

            for (CWindow* member : TabGroup(window)) {
                member->dock_cross_size = size;
            }

            break;
        }

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

            SetDockSize(window, std::max(size, Vertical(window.dock) ? window.min_size.y : window.min_size.x));
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
        screen.Put(right, bottom, window.dock == EDock::None && !window.hasOption(EWindowOptions::NoResize) ? U'◢' : U'┘', colour);

        // Reserve room for the close button only when closing is enabled.
        const bool closable = !window.hasOption(EWindowOptions::NoClose) && (allow_tab_closing || window.tab_group == 0);

        if (closable) {
            screen.Put(right - 1, frame.corner.y, U'×', colour);
        }

        screen.setClip({Position(frame.corner.x + 1, frame.corner.y), Size(frame.size.x - (closable ? 3 : 2), 1)});
        screen.Print(frame.corner.x + 1, frame.corner.y, " " + window.getFrameTitle() + " ", focused ? style.focused_title : style.title);
        screen.resetClip();
    }

    // Tracks occupy the right and bottom borders; content and corners stay intact.
    void DrawScrollIndicators(CScreen& screen, const CWindow& window, const SRect& inside) const
    {
        const auto info = window.getScrollInfo();

        if (!info || inside.Empty()) {
            return;
        }

        const std::uint32_t colour = &window == Top() ? style.focused_frame : style.frame;
        const auto axis = [&](bool vertical, std::size_t content, std::size_t viewport, std::size_t offset) {
            const std::size_t length = vertical ? inside.size.y : inside.size.x;

            if (viewport == 0 || content <= viewport || length == 0) {
                return;
            }

            const auto geometry = ScrollAxis(length, content, viewport, offset);
            const std::size_t thumb = geometry.thumb;
            const std::size_t start = geometry.start;

            for (std::size_t index = 0; index < length; ++index) {
                const bool selected = index >= start && index - start < thumb;
                screen.Put(vertical ? window.frame.Right() - 1 : inside.corner.x + index,
                           vertical ? inside.corner.y + index : window.frame.Bottom() - 1,
                           selected ? U' ' : vertical ? U'│' : U'─', colour, false, selected);
            }
        };

        axis(true, info->content.y, info->viewport.y, info->offset.y);
        axis(false, info->content.x, info->viewport.x, info->offset.x);
    }

    // Draws the tabs of a group in the strip under the title, and the
    // rule under them that the frame's sides turn into
    void DrawTabs(CScreen& screen, const std::vector<CWindow*>& group, const CWindow& showing, bool focused) const
    {
        const SRect& frame = showing.frame;
        const std::size_t strip = frame.corner.y + 1;
        const std::size_t rule = strip + 1;
        const std::size_t right = frame.Right() - 1;
        const std::uint32_t colour = focused ? style.focused_frame : style.frame;

        screen.Put(frame.corner.x, rule, U'├', colour);
        screen.Put(right, rule, U'┤', colour);

        for (std::size_t x = frame.corner.x + 1; x < right; ++x) {
            screen.Put(x, rule, U'─', colour);
        }

        for (const STab& tab : TabsIn(group, frame)) {
            // The top of the box the tab is in; its sides come down to
            // the rule, which closes it there
            screen.Put(tab.x, strip, U'┌', colour);
            screen.Put(tab.x + tab.size - 1, strip, U'┐', colour);

            for (std::size_t x = tab.x + 1; x + 1 < tab.x + tab.size; ++x) {
                screen.Put(x, strip, U'─', colour);
            }

            // The tab that shows takes its title bold and inverse, so
            // that it stands out from the others of the strip
            const bool active = tab.window == &showing;

            screen.setClip({Position(tab.x + 1, strip), Size(tab.size - 2, 1)});
            screen.Print(tab.x + 1, strip, " " + tab.window->title + " ",
                         active && focused ? style.focused_title : style.title, active, active);
            screen.resetClip();

            // The rule meets the sides of the box coming down to it
            screen.Put(tab.x, rule, U'┴', colour);
            screen.Put(tab.x + tab.size - 1, rule, U'┴', colour);
        }
    }

public:
    // Whether tab-group windows show and respond to a close button.
    // Enabled by default. Explicit Close/Toggle calls remain available.
    void setAllowTabClosing(bool allow)
    {
        allow_tab_closing = allow;
    }

    [[nodiscard]] bool getAllowTabClosing() const
    {
        return allow_tab_closing;
    }

    // Whether dragging a docked tab's title can tear it out of its group.
    // Enabled by default. Explicit Dock/Untab calls remain available.
    void setAllowTabUndocking(bool allow)
    {
        allow_tab_undocking = allow;

        if (!allow && drag && drag->grab == EGrab::Undock && drag->window->tab_group != 0) {
            drag.reset();
        }
    }

    [[nodiscard]] bool getAllowTabUndocking() const
    {
        return allow_tab_undocking;
    }

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

        if (scroll_drag && scroll_drag->window == &window) {
            scroll_drag.reset();
        }

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

    // Focus follows stacking order: opening, raising or clicking a window
    // focuses it and selects its tab. Closing it focuses the next on top.
    [[nodiscard]] CWindow* getFocused() const
    {
        return Top();
    }

    // Explicitly focuses an open window; closed windows stay closed.
    bool Focus(CWindow& window)
    {
        if (!IsOpen(window)) {
            return false;
        }

        Raise(window);
        return true;
    }

    // Routes keyboard events only. Mouse, resize, closure and timeout
    // events remain the application's responsibility. No key is reserved
    // for switching focus, so widgets may use Tab or arrows themselves.
    bool HandleKey(const SKey& key)
    {
        if (key.type == EKey::None || key.type == EKey::Mouse ||
                key.type == EKey::Resize || key.type == EKey::Closed) {
            return false;
        }

        CWindow* focused = getFocused();
        return focused != nullptr && focused->HandleKey(key);
    }

    // Docks a window to an edge, as the innermost of those docked there.
    // `size` is its rows docked to the top or bottom, its columns to the
    // left or right, frame included. EDock::None floats it again.
    void Dock(CWindow& window, EDock dock, std::size_t size)
    {
        window.dock = dock;
        window.dock_cross_size = 0;
        window.dock_size = std::max(size, Vertical(dock) ? window.min_size.y : window.min_size.x);
        window.docked_at = next_docked_at++;
    }

    // Puts a window in the tab group of another: docked to the same
    // edge, the two then share one slot and one frame, one of them
    // showing at a time behind tabs under its title. Docked elsewhere,
    // each keeps a frame of its own until they meet at an edge again
    void Tab(CWindow& window, CWindow& with)
    {
        if (&window == &with) {
            return;
        }

        if (with.tab_group == 0) {
            with.tab_group = next_tab_group++;
        }

        window.tab_group = with.tab_group;
    }

    // Takes a window out of its tab group, docked where it was
    void Untab(CWindow& window)
    {
        window.tab_group = 0;
    }

    // Where a window is and how it is shown
    [[nodiscard]] SWindowLayout getLayout(const CWindow& window) const
    {
        return {IsOpen(window), window.dock, window.dock_size, window.docked_at, window.floating, window.tab_group, window.dock_cross_size};
    }

    // Puts a window where a layout says, opening or closing it
    void setLayout(CWindow& window, const SWindowLayout& layout)
    {
        window.floating = layout.floating;
        window.dock_cross_size = layout.dock_cross_size;
        window.dock = layout.dock;
        window.dock_size = layout.dock == EDock::None ? layout.dock_size :
                           std::max(layout.dock_size, Vertical(layout.dock) ? window.min_size.y : window.min_size.x);
        window.docked_at = layout.docked_at;
        next_docked_at = std::max(next_docked_at, layout.docked_at + 1);
        window.tab_group = layout.tab_group;
        next_tab_group = std::max(next_tab_group, layout.tab_group + 1);

        if (layout.open) {
            Open(window);
        } else {
            Close(window);
        }
    }

    // Lays the open windows out in an area of the screen: the docked ones
    // along its edges, the floating ones within it. Returns what is left
    // between the docked ones. A tab group is laid out as one window in
    // one slot, as large as the largest of it asks for.
    SRect Layout(const SRect& a_area)
    {
        area = a_area;
        SRect free = area;
        std::vector<CWindow*> laid;

        for (CWindow* window : InDrawingOrder()) {
            if (window->dock == EDock::None) {
                window->frame = Clamp(window->floating, window->min_size);
                continue;
            }

            // The first of a tab group to come decides where the slot is,
            // the rest of them skip along to be laid out with it
            std::vector<CWindow*> slot{window};
            const std::vector<CWindow*> group = TabGroup(*window);

            if (!group.empty()) {
                if (std::ranges::find(laid, window) != laid.end()) {
                    continue;
                }

                slot = group;
            }

            for (CWindow* member : slot) {
                laid.push_back(member);
            }

            const bool vertical = Vertical(window->dock);
            const std::size_t room = vertical ? free.size.y : free.size.x;
            const std::size_t across = vertical ? free.size.x : free.size.y;

            // What the slot has to fit at least: the largest of the
            // windows in it along the edge and across it, and as many
            // rows or columns as the largest of them wants
            std::size_t least = 0;
            std::size_t least_across = 0;
            std::size_t wanted = 0;
            std::size_t wanted_across = 0;

            for (CWindow* member : slot) {
                least = std::max(least, vertical ? member->min_size.y : member->min_size.x);
                least_across = std::max(least_across, vertical ? member->min_size.x : member->min_size.y);
                wanted = std::max(wanted, member->dock_size);
                wanted_across = std::max(wanted_across, member->dock_cross_size == 0 ? across : member->dock_cross_size);
            }

            // No room for it: it is not shown until there is
            if (room < least || across < least_across) {
                for (CWindow* member : slot) {
                    member->frame = {free.corner, Size(0, 0)};
                }

                continue;
            }

            // The tabs take as much as the largest of them, so that
            // switching one does not move the frame
            for (CWindow* member : slot) {
                member->dock_size = wanted;
            }

            const std::size_t size = std::min(wanted, room);
            const std::size_t cross_size = std::min(std::max(wanted_across, least_across), across);
            SRect frame{free.corner, Size(0, 0)};

            switch (window->dock) {
            case EDock::Top:
                frame = {free.corner, Size(cross_size, size)};
                free.corner.y += size;
                free.size.y -= size;
                break;

            case EDock::Bottom:
                frame = {Position(free.corner.x, free.Bottom() - size), Size(cross_size, size)};
                free.size.y -= size;
                break;

            case EDock::Left:
                frame = {free.corner, Size(size, cross_size)};
                free.corner.x += size;
                free.size.x -= size;
                break;

            case EDock::Right:
                frame = {Position(free.Right() - size, free.corner.y), Size(size, cross_size)};
                free.size.x -= size;
                break;

            case EDock::None:
                break;
            }

            for (CWindow* member : slot) {
                member->frame = frame;
            }
        }

        return free;
    }

    // Draws the open windows as laid out last, over what is on the screen.
    // A tab group is drawn once, as the frame of all of it with the tabs
    // under its title and the content of the one that shows
    void Draw(CScreen& screen) const
    {
        std::vector<const CWindow*> drawn;

        for (CWindow* window : InDrawingOrder()) {
            if (std::ranges::find(drawn, window) != drawn.end()) {
                continue;   // it drew with the others of its tabs
            }

            const SRect& frame = window->frame;

            if (frame.size.x < 5 || frame.size.y < 3) {
                continue;
            }

            const std::vector<CWindow*> group = TabGroup(*window);
            CWindow* showing = group.empty() ? window : Active(group);

            for (CWindow* member : group) {
                drawn.push_back(member);
            }

            // What is inside the frame, tabs included: it is cleared
            // first, so that nothing of an earlier frame shows through
            const SRect interior = frame.Inset(1);

            for (std::size_t y = interior.corner.y; y < interior.Bottom(); ++y) {
                for (std::size_t x = interior.corner.x; x < interior.Right(); ++x) {
                    screen.Put(x, y, U' ', default_text_colour);
                }
            }

            DrawFrame(screen, *showing, showing == Top());

            if (HasTabs(*showing)) {
                DrawTabs(screen, group, *showing, showing == Top());
            }

            const SRect inside = ContentRect(*showing);

            screen.setClip(inside);
            showing->Draw(screen, inside);
            screen.resetClip();
            DrawScrollIndicators(screen, *showing, inside);
        }
    }

    // Moves, resizes, docks, closes and scrolls windows as the mouse says.
    // Returns whether it was meant for a window, rather than for what is
    // under them. Told when the mouse did it, two clicks on a title in
    // quick succession are a double-click; without, there are none.
    bool HandleMouse(const SMouse& mouse, std::optional<std::chrono::steady_clock::time_point> now = std::nullopt)
    {
        const Position pos = mouse.position;

        if (scroll_drag) {
            if (mouse.action == EMouseAction::Drag || mouse.action == EMouseAction::Release) {
                DragScroll(pos);
            }

            if (mouse.action == EMouseAction::Release) {
                scroll_drag.reset();
            }

            return true;
        }

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
            if (!PressScroll(*window, mouse)) {
                Press(*window, mouse, now);
            }

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

//////////////////////////////////////////////////////////////////////////
// Scrollbar
//
// A generalized scrollbar component for lists, text views, and windows.
// Supports vertical and horizontal orientations, configurable arrows,
// track and thumb styling, proportional thumb geometry, and mouse
// interactions (clicking arrows for single steps, clicking track for
// paging, and thumb dragging).
//////////////////////////////////////////////////////////////////////////

module;

export module rtui.scrollbar;

export import rtui.windows;
export import rtui.screen;
export import rtui.mouse;
export import rl.position;
import std;

export namespace RL
{

/**
 * @brief Scrollbar orientation.
 */
enum class EScrollbarOrientation {
    Vertical,
    Horizontal
};

/**
 * @brief Visual and glyph configuration for a scrollbar.
 */
struct SScrollbarStyle {
    char32_t arrow_dec = U'▲';           ///< Decrement arrow glyph (Up for vertical, Left for horizontal).
    char32_t arrow_inc = U'▼';           ///< Increment arrow glyph (Down for vertical, Right for horizontal).
    char32_t track_glyph = U'│';         ///< Track glyph between arrows.
    char32_t thumb_glyph = U'█';         ///< Thumb slider glyph.
    char32_t empty_track_glyph = U'│';   ///< Track glyph when content fits in viewport.

    std::uint32_t arrow_active = 0xFFFF70;   ///< Color of arrow when scrolling in that direction is available.
    std::uint32_t arrow_inactive = 0x505050; ///< Color of arrow when at boundary limit.
    std::uint32_t track_color = 0x404040;    ///< Color of the background track.
    std::uint32_t thumb_color = 0xA0A0A0;    ///< Color of the thumb slider.
    std::uint32_t empty_track_color = 0x505050; ///< Color of the track when inactive/empty.

    bool show_arrows = true;             ///< Whether to render end arrow buttons.
};

/**
 * @brief Action resulting from a scrollbar click.
 */
enum class EScrollAction {
    None,       ///< Click missed or scrollbar inactive.
    LineDec,    ///< Decrement by one unit (e.g. up arrow or left arrow).
    LineInc,    ///< Increment by one unit (e.g. down arrow or right arrow).
    PageDec,    ///< Decrement by page/viewport size (clicked track before thumb).
    PageInc,    ///< Increment by page/viewport size (clicked track after thumb).
    Thumb       ///< Clicked on the thumb slider (starts dragging).
};

/**
 * @brief Calculated scrollbar geometry parameters.
 */
struct SScrollbarGeometry {
    std::size_t length = 0;              ///< Total length in cells.
    std::size_t track_length = 0;        ///< Track length excluding arrows.
    std::size_t thumb_length = 0;        ///< Thumb size in cells.
    std::size_t thumb_start = 0;         ///< Thumb start position along track (0-indexed).
    std::size_t max_offset = 0;          ///< Maximum valid content offset.
    bool can_scroll_dec = false;         ///< True if offset > 0.
    bool can_scroll_inc = false;         ///< True if offset + viewport < content.
    bool active = false;                 ///< True if content > viewport and length >= minimum.
};

/**
 * @brief Result of a mouse click on a scrollbar.
 */
struct SScrollbarClickResult {
    EScrollAction action = EScrollAction::None;
    int delta = 0;                       ///< Suggested offset delta (-1, +1, -viewport, +viewport).
};

/**
 * @brief Generalized scrollbar component.
 */
class CScrollbar
{
private:
    SRect area{Position(0, 0), Size(1, 0)};
    EScrollbarOrientation orientation = EScrollbarOrientation::Vertical;
    SScrollbarStyle style{};

    bool dragging = false;
    std::size_t drag_grab = 0;

public:
    CScrollbar(EScrollbarOrientation orient = EScrollbarOrientation::Vertical,
               SScrollbarStyle a_style = {})
        : orientation(orient), style(a_style)
    {
        if (orientation == EScrollbarOrientation::Horizontal && a_style.arrow_dec == U'▲') {
            style.arrow_dec = U'◄';
            style.arrow_inc = U'►';
            style.track_glyph = U'─';
            style.empty_track_glyph = U'─';
        }
    }

    CScrollbar(const SRect& a_area,
               EScrollbarOrientation orient = EScrollbarOrientation::Vertical,
               SScrollbarStyle a_style = {})
        : CScrollbar(orient, a_style)
    {
        area = a_area;
    }

    [[nodiscard]] const SRect& getArea() const noexcept
    {
        return area;
    }

    void setArea(const SRect& a_area) noexcept
    {
        area = a_area;
    }

    [[nodiscard]] EScrollbarOrientation getOrientation() const noexcept
    {
        return orientation;
    }

    void setOrientation(EScrollbarOrientation orient) noexcept
    {
        orientation = orient;
        if (orientation == EScrollbarOrientation::Horizontal && style.arrow_dec == U'▲') {
            style.arrow_dec = U'◄';
            style.arrow_inc = U'►';
            style.track_glyph = U'─';
            style.empty_track_glyph = U'─';
        }
    }

    [[nodiscard]] const SScrollbarStyle& getStyle() const noexcept
    {
        return style;
    }

    void setStyle(const SScrollbarStyle& a_style) noexcept
    {
        style = a_style;
    }

    /**
     * @brief Automatically positions the scrollbar adjacent to a view area.
     *
     * For vertical scrollbars, places a 1-column scrollbar along the right edge:
     * `(view_rect.Right(), view_rect.corner.y)` with height `view_rect.size.y`.
     * For horizontal scrollbars, places a 1-row scrollbar along the bottom edge:
     * `(view_rect.corner.x, view_rect.Bottom())` with width `view_rect.size.x`.
     */
    void AttachTo(const SRect& view_rect) noexcept
    {
        if (orientation == EScrollbarOrientation::Vertical) {
            area = SRect(Position(view_rect.Right(), view_rect.corner.y), Size(1, view_rect.size.y));
        } else {
            area = SRect(Position(view_rect.corner.x, view_rect.Bottom()), Size(view_rect.size.x, 1));
        }
    }

    [[nodiscard]]
    bool Contains(Position pos) const noexcept
    {
        return area.Contains(pos);
    }

    [[nodiscard]]
    SScrollbarGeometry CalculateGeometry(std::size_t content, std::size_t viewport, std::size_t offset) const
    {
        const std::size_t len = (orientation == EScrollbarOrientation::Vertical) ? area.size.y : area.size.x;
        const std::size_t min_len = style.show_arrows ? 3 : 1;
        if (len < min_len || viewport == 0 || content <= viewport) {
            return SScrollbarGeometry{
                .length = len,
                .track_length = style.show_arrows && len >= 2 ? len - 2 : len,
                .thumb_length = 0,
                .thumb_start = 0,
                .max_offset = content > viewport ? content - viewport : 0,
                .can_scroll_dec = false,
                .can_scroll_inc = false,
                .active = false
            };
        }

        const std::size_t track_len = style.show_arrows ? (len - 2) : len;
        const std::size_t thumb_len = std::clamp(static_cast<std::size_t>(
            static_cast<double>(viewport) / content * track_len), std::size_t{1}, track_len);
        const std::size_t max_off = content - viewport;
        const std::size_t thumb_st = (max_off == 0) ? 0 : static_cast<std::size_t>(
            static_cast<double>(std::min(offset, max_off)) / max_off * (track_len - thumb_len));

        return SScrollbarGeometry{
            .length = len,
            .track_length = track_len,
            .thumb_length = thumb_len,
            .thumb_start = thumb_st,
            .max_offset = max_off,
            .can_scroll_dec = offset > 0,
            .can_scroll_inc = offset + viewport < content,
            .active = true
        };
    }

    [[nodiscard]]
    SScrollbarGeometry CalculateGeometry(const SScrollInfo& info) const
    {
        if (orientation == EScrollbarOrientation::Vertical) {
            return CalculateGeometry(info.content.y, info.viewport.y, info.offset.y);
        } else {
            return CalculateGeometry(info.content.x, info.viewport.x, info.offset.x);
        }
    }

    void Draw(CScreen& screen, std::size_t content, std::size_t viewport, std::size_t offset) const
    {
        const auto geom = CalculateGeometry(content, viewport, offset);
        const std::size_t len = geom.length;
        if (len == 0) return;

        const bool vert = (orientation == EScrollbarOrientation::Vertical);

        if (!geom.active) {
            for (std::size_t i = 0; i < len; ++i) {
                const std::size_t x = vert ? area.corner.x : area.corner.x + i;
                const std::size_t y = vert ? area.corner.y + i : area.corner.y;
                screen.Put(x, y, style.empty_track_glyph, style.empty_track_color);
            }
            return;
        }

        if (style.show_arrows) {
            const std::size_t x0 = area.corner.x;
            const std::size_t y0 = area.corner.y;
            screen.Put(x0, y0, style.arrow_dec,
                       geom.can_scroll_dec ? style.arrow_active : style.arrow_inactive,
                       geom.can_scroll_dec);

            const std::size_t x1 = vert ? area.corner.x : area.corner.x + len - 1;
            const std::size_t y1 = vert ? area.corner.y + len - 1 : area.corner.y;
            screen.Put(x1, y1, style.arrow_inc,
                       geom.can_scroll_inc ? style.arrow_active : style.arrow_inactive,
                       geom.can_scroll_inc);
        }

        const std::size_t track_offset = style.show_arrows ? 1 : 0;
        for (std::size_t i = 0; i < geom.track_length; ++i) {
            const std::size_t x = vert ? area.corner.x : area.corner.x + track_offset + i;
            const std::size_t y = vert ? area.corner.y + track_offset + i : area.corner.y;

            if (i >= geom.thumb_start && i < geom.thumb_start + geom.thumb_length) {
                screen.Put(x, y, style.thumb_glyph, style.thumb_color);
            } else {
                screen.Put(x, y, style.track_glyph, style.track_color);
            }
        }
    }

    void Draw(CScreen& screen, const SScrollInfo& info) const
    {
        if (orientation == EScrollbarOrientation::Vertical) {
            Draw(screen, info.content.y, info.viewport.y, info.offset.y);
        } else {
            Draw(screen, info.content.x, info.viewport.x, info.offset.x);
        }
    }

    SScrollbarClickResult Click(Position pos, std::size_t content, std::size_t viewport, std::size_t offset)
    {
        if (!Contains(pos)) {
            return {};
        }

        const auto geom = CalculateGeometry(content, viewport, offset);
        if (!geom.active) {
            return {};
        }

        const bool vert = (orientation == EScrollbarOrientation::Vertical);
        const std::size_t coord = vert ? (pos.y - area.corner.y) : (pos.x - area.corner.x);

        if (style.show_arrows) {
            if (coord == 0) {
                return SScrollbarClickResult{.action = EScrollAction::LineDec, .delta = -1};
            }
            if (coord == geom.length - 1) {
                return SScrollbarClickResult{.action = EScrollAction::LineInc, .delta = 1};
            }
        }

        const std::size_t track_pos = style.show_arrows ? (coord - 1) : coord;
        if (track_pos < geom.thumb_start) {
            return SScrollbarClickResult{.action = EScrollAction::PageDec, .delta = -static_cast<int>(viewport)};
        }
        if (track_pos >= geom.thumb_start + geom.thumb_length) {
            return SScrollbarClickResult{.action = EScrollAction::PageInc, .delta = static_cast<int>(viewport)};
        }

        dragging = true;
        drag_grab = track_pos - geom.thumb_start;
        return SScrollbarClickResult{.action = EScrollAction::Thumb, .delta = 0};
    }

    SScrollbarClickResult Click(Position pos, const SScrollInfo& info)
    {
        if (orientation == EScrollbarOrientation::Vertical) {
            return Click(pos, info.content.y, info.viewport.y, info.offset.y);
        } else {
            return Click(pos, info.content.x, info.viewport.x, info.offset.x);
        }
    }

    std::optional<std::size_t> Drag(Position pos, std::size_t content, std::size_t viewport, std::size_t offset)
    {
        if (!dragging) {
            return std::nullopt;
        }

        const auto geom = CalculateGeometry(content, viewport, offset);
        if (!geom.active || geom.track_length <= geom.thumb_length) {
            return std::nullopt;
        }

        const bool vert = (orientation == EScrollbarOrientation::Vertical);
        const std::size_t coord = vert ? pos.y : pos.x;
        const std::size_t corner = vert ? area.corner.y : area.corner.x;
        const std::size_t track_start = corner + (style.show_arrows ? 1 : 0);

        const std::size_t relative = (coord > track_start) ? (coord - track_start) : 0;
        const std::size_t travel = geom.track_length - geom.thumb_length;
        const std::size_t start = std::min(
            (relative > drag_grab) ? (relative - drag_grab) : 0,
            travel
        );

        const double ratio = static_cast<double>(start) / travel;
        const std::size_t new_offset = static_cast<std::size_t>(std::round(ratio * geom.max_offset));
        return new_offset;
    }

    std::optional<std::size_t> Drag(Position pos, const SScrollInfo& info)
    {
        if (orientation == EScrollbarOrientation::Vertical) {
            return Drag(pos, info.content.y, info.viewport.y, info.offset.y);
        } else {
            return Drag(pos, info.content.x, info.viewport.x, info.offset.x);
        }
    }

    void Release() noexcept
    {
        dragging = false;
        drag_grab = 0;
    }

    [[nodiscard]] bool isDragging() const noexcept
    {
        return dragging;
    }
};

/**
 * @brief Standalone helper to draw a scrollbar directly to screen.
 */
inline void DrawScrollbar(CScreen& screen, const SRect& area, const SScrollInfo& info,
                          EScrollbarOrientation orientation = EScrollbarOrientation::Vertical,
                          const SScrollbarStyle& style = {})
{
    CScrollbar bar(area, orientation, style);
    bar.Draw(screen, info);
}

} // namespace RL

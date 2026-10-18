// Tests for RTUI's rtui.scrollbar: generalized scrollbar component

import rtui.scrollbar;
import rltests.check;
import std;

using RL::tests::Check;
using RL::tests::Row;

void TestScrollbarGeometry()
{
    RL::CScrollbar bar(RL::SRect(RL::Position(0, 0), RL::Size(1, 10)));

    // When content fits in viewport, scrollbar is inactive
    const auto inactive_geom = bar.CalculateGeometry(5, 10, 0);
    Check(!inactive_geom.active, "scrollbar inactive when content fits in viewport");

    // When length is too small (< 3 with arrows), scrollbar is inactive
    RL::CScrollbar small_bar(RL::SRect(RL::Position(0, 0), RL::Size(1, 2)));
    Check(!small_bar.CalculateGeometry(20, 5, 0).active, "scrollbar inactive when area is smaller than minimum arrow length");

    // Standard vertical scrollbar: 20 content, 5 viewport, 10 total height (8 track)
    const auto top_geom = bar.CalculateGeometry(20, 5, 0);
    Check(top_geom.active, "scrollbar active when content exceeds viewport");
    Check(top_geom.track_length == 8, "track length excludes 2 arrow buttons");
    Check(top_geom.thumb_length == 2, "thumb length proportionally clamped (5/20 * 8 = 2)");
    Check(top_geom.thumb_start == 0, "thumb starts at top when offset is 0");
    Check(!top_geom.can_scroll_dec && top_geom.can_scroll_inc, "can scroll down but not up at top");

    // Scrolled to bottom: max_offset = 15
    const auto bottom_geom = bar.CalculateGeometry(20, 5, 15);
    Check(bottom_geom.thumb_start == 6, "thumb starts at end of track (8 - 2 = 6)");
    Check(bottom_geom.can_scroll_dec && !bottom_geom.can_scroll_inc, "can scroll up but not down at bottom");

    // Scrolled midway: offset = 7
    const auto mid_geom = bar.CalculateGeometry(20, 5, 7);
    Check(mid_geom.can_scroll_dec && mid_geom.can_scroll_inc, "can scroll both ways in the middle");
    Check(mid_geom.thumb_start > 0 && mid_geom.thumb_start < 6, "thumb is positioned in the middle");
}

void TestScrollbarDrawing()
{
    RL::CScreen screen(RL::Size(1, 10));
    RL::CScrollbar bar(RL::SRect(RL::Position(0, 0), RL::Size(1, 10)));

    // 1. Draw active vertical scrollbar at offset 0
    bar.Draw(screen, 20, 5, 0);
    Check(screen.At(0, 0).glyph == U'▲', "top row renders up arrow");
    Check(!screen.At(0, 0).bold, "top arrow is dimmed when cannot scroll up");
    Check(screen.At(0, 1).glyph == U'█', "row 1 renders thumb block");
    Check(screen.At(0, 2).glyph == U'█', "row 2 renders thumb block");
    Check(screen.At(0, 3).glyph == U'│', "row 3 renders track");
    Check(screen.At(0, 8).glyph == U'│', "row 8 renders track");
    Check(screen.At(0, 9).glyph == U'▼', "bottom row renders down arrow");
    Check(screen.At(0, 9).bold, "bottom arrow is highlighted when can scroll down");

    // 2. Draw inactive scrollbar (content fits in viewport)
    screen.Clear();
    bar.Draw(screen, 5, 10, 0);
    bool all_track = true;
    for (std::size_t y = 0; y < 10; ++y) {
        all_track &= (screen.At(0, y).glyph == U'│');
    }
    Check(all_track, "inactive scrollbar renders full track without arrows");

    // 3. Draw horizontal scrollbar
    RL::CScreen h_screen(RL::Size(10, 1));
    RL::CScrollbar h_bar(RL::SRect(RL::Position(0, 0), RL::Size(10, 1)), RL::EScrollbarOrientation::Horizontal);
    h_bar.Draw(h_screen, 20, 5, 0);
    Check(h_screen.At(0, 0).glyph == U'◄', "horizontal scrollbar renders left arrow");
    Check(h_screen.At(1, 0).glyph == U'█', "horizontal thumb renders block");
    Check(h_screen.At(3, 0).glyph == U'─', "horizontal track renders horizontal line");
    Check(h_screen.At(9, 0).glyph == U'►', "horizontal scrollbar renders right arrow");
}

void TestScrollbarInteraction()
{
    RL::CScrollbar bar(RL::SRect(RL::Position(5, 2), RL::Size(1, 10)));
    const std::size_t content = 20;
    const std::size_t viewport = 5;
    const std::size_t offset = 0;

    // Click outside
    Check(bar.Click(RL::Position(0, 0), content, viewport, offset).action == RL::EScrollAction::None,
          "clicking outside returns None");

    // Click up arrow at (5, 2)
    const auto up_res = bar.Click(RL::Position(5, 2), content, viewport, offset);
    Check(up_res.action == RL::EScrollAction::LineDec && up_res.delta == -1,
          "clicking top arrow returns LineDec with delta -1");

    // Click down arrow at (5, 11)
    const auto down_res = bar.Click(RL::Position(5, 11), content, viewport, offset);
    Check(down_res.action == RL::EScrollAction::LineInc && down_res.delta == 1,
          "clicking bottom arrow returns LineInc with delta 1");

    // Click track below thumb at (5, 6)
    const auto page_res = bar.Click(RL::Position(5, 6), content, viewport, offset);
    Check(page_res.action == RL::EScrollAction::PageInc && page_res.delta == 5,
          "clicking track below thumb returns PageInc with delta equal to viewport");

    // Click thumb at (5, 3) starts drag
    const auto thumb_res = bar.Click(RL::Position(5, 3), content, viewport, offset);
    Check(thumb_res.action == RL::EScrollAction::Thumb && bar.isDragging(),
          "clicking thumb returns Thumb action and begins drag");

    // Drag down to (5, 8)
    const auto new_offset = bar.Drag(RL::Position(5, 8), content, viewport, offset);
    Check(new_offset.has_value() && *new_offset > 0, "dragging computes proportional new offset");

    bar.Release();
    Check(!bar.isDragging(), "Release ends dragging");
}

void TestScrollbarAttachment()
{
    RL::SRect view(RL::Position(2, 4), RL::Size(20, 10));

    RL::CScrollbar v_bar(RL::EScrollbarOrientation::Vertical);
    v_bar.AttachTo(view);
    Check(v_bar.getArea().corner == RL::Position(22, 4) && v_bar.getArea().size == RL::Size(1, 10),
          "AttachTo places vertical scrollbar adjacent to view right border");

    RL::CScrollbar h_bar(RL::EScrollbarOrientation::Horizontal);
    h_bar.AttachTo(view);
    Check(h_bar.getArea().corner == RL::Position(2, 14) && h_bar.getArea().size == RL::Size(20, 1),
          "AttachTo places horizontal scrollbar adjacent to view bottom border");
}

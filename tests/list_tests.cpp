// Tests for RTUI's rtui.list: a list of items to choose from

import rtui.list;
import rltests.check;
import std;

using RL::tests::Check;
using RL::tests::Row;

// Items as literal text, or as text with colour markup
void TestListMarkup()
{
    RL::CScreen screen(RL::Size(12, 3));
    screen.RegisterTag("key", 0x00A6A6);
    const RL::SRect inside{RL::Position(0, 0), RL::Size(12, 3)};

    RL::CListWindow literal("Items", inside, {"a<key>b", "c"});
    literal.Draw(screen, inside);
    Check(Row(screen, 0) == U"a<key>b     " && screen.At(0, 0).inverse && !screen.At(0, 1).inverse,
          "literal items show as written, the selected one highlighted");

    screen.Clear();
    RL::CListWindow coloured("Items", inside, {"a<key>b", "<#102030>c"});
    coloured.setMarkup(true);
    coloured.Draw(screen, inside);
    Check(coloured.getMarkup() && Row(screen, 0) == U"ab          " && screen.At(1, 0).rgb == 0x00A6A6 &&
          screen.At(0, 1).rgb == 0x102030, "items with markup show in their colours");
    Check(coloured.getScrollInfo()->content.x == 2, "and are as wide as what shows of them");

    // Scrolled sideways, an item keeps the colour it has where it is cut
    screen.Clear();
    RL::CListWindow wide("Items", inside, {"<key>abcdefghijklmnop"});
    wide.setMarkup(true);
    wide.Draw(screen, inside);
    wide.setScrollOffset(RL::Position(3, 0));
    wide.Draw(screen, inside);
    Check(Row(screen, 0) == U"defghijklmno" && screen.At(0, 0).rgb == 0x00A6A6,
          "scrolled sideways, an item keeps its colour");
}

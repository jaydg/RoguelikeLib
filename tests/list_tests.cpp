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

// A disabled item shows dimmed, and can be selected, but not activated
void TestListDisabled()
{
    RL::CScreen screen(RL::Size(12, 3));
    const RL::SRect inside{RL::Position(0, 0), RL::Size(12, 3)};
    RL::CListWindow list("Items", inside, {"one", "two", "three"});
    std::vector<std::size_t> activated;
    list.setOnActivate([&](std::size_t index) {
        activated.push_back(index);
    });

    list.setEnabled(1, false);
    list.setEnabled(7, false);
    Check(list.isEnabled(0) && !list.isEnabled(1) && list.isEnabled(2) && !list.isEnabled(7),
          "an item is disabled; one that is not there, never enabled");

    list.Draw(screen, inside);
    Check(screen.At(0, 1).rgb == RL::Dim(screen.getTextColour(), 50) && screen.At(0, 0).rgb == screen.getTextColour(),
          "a disabled item shows dimmed");

    Check(list.HandleKey(RL::SKey{RL::EKey::Down, 0, {}}) && list.getSelected() == std::optional<std::size_t>(1),
          "it can be selected, to look at it");
    Check(!list.HandleKey(RL::SKey{RL::EKey::Enter, 0, {}}) && activated.empty(), "but Enter does not activate it");
    list.Click(RL::Position(0, 1), RL::EMouseButton::Left);
    Check(activated.empty() && list.getSelected() == std::optional<std::size_t>(1), "nor does a click");
    list.Click(RL::Position(0, 2), RL::EMouseButton::Left);
    Check(activated == std::vector<std::size_t> {2}, "an enabled one is activated");

    list.setEnabled(1, true);
    list.Select(1);
    Check(list.Activate() && activated.back() == 1, "enabled again, it is activated too");
    list.setEnabled(0, false);
    list.setItems({"new", "items"});
    Check(list.isEnabled(0) && list.isEnabled(1), "a new list of items has them all enabled");
}

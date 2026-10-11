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

// Whoever follows the selection is told of every item selected, however
void TestListFollowed()
{
    const RL::SRect inside{RL::Position(0, 0), RL::Size(12, 3)};
    RL::CListWindow list("Items", inside, {"zero", "one", "two", "three", "four", "five"});
    std::vector<std::size_t> followed;
    list.setOnSelect([&](std::size_t index) {
        followed.push_back(index);
    });

    RL::CScreen screen(RL::Size(12, 3));
    list.Draw(screen, inside);
    list.HandleKey(RL::SKey{RL::EKey::Down, 0, {}});
    list.HandleKey(RL::SKey{RL::EKey::End, 0, {}});
    list.Scroll(-1);
    list.Click(RL::Position(0, 0), RL::EMouseButton::Left);
    list.Select(5);
    Check(followed == std::vector<std::size_t> {1, 5, 4, 3, 5}, "keys, the wheel, a click and Select() are followed");

    followed.clear();
    list.Select(5);
    list.Select(9);
    list.HandleKey(RL::SKey{RL::EKey::Down, 0, {}});
    Check(followed.empty(), "selecting what is selected already, or nothing, is not");

    list.setScrollOffset(RL::Position(0, 0));
    Check(followed == std::vector<std::size_t> {2}, "the scroll bar dragged away from the selection takes it along, and that is followed too");

    followed.clear();
    list.setItems({"new", "items"});
    list.setItems({});
    Check(followed == std::vector<std::size_t> {0}, "a new list is followed from its first item; an empty one has none");
}

// Items with keys of their own: typed, a key selects its item, and
// activates it unless it is disabled
void TestListShortcuts()
{
    RL::CScreen screen(RL::Size(10, 3));
    const RL::SRect inside{RL::Position(0, 0), RL::Size(10, 3)};
    RL::CListWindow list("Items", inside, {"one", "two", "three"});
    std::vector<std::size_t> activated;
    list.setOnActivate([&](std::size_t index) {
        activated.push_back(index);
    });
    const auto type = [&](char32_t key) {
        return list.HandleKey(RL::SKey{RL::EKey::Char, key, {}});
    };

    list.Draw(screen, inside);
    Check(Row(screen, 0) == U"one       " && list.getShortcut(0) == 0 && !type(U'a'),
          "without keys, items show as they are, and a letter is not taken");

    list.setLetterShortcuts();
    list.setEnabled(2, false);
    list.Draw(screen, inside);
    Check(list.getShortcut(0) == U'a' && list.getShortcut(2) == U'c' && Row(screen, 1) == U"b two     " &&
          screen.At(0, 1).bold && !screen.At(2, 1).bold, "given letters, they show their keys before them");
    Check(type(U'b') && list.getSelected() == std::optional<std::size_t>(1) && activated == std::vector<std::size_t> {1},
          "a key typed selects and activates its item");
    Check(type(U'c') && list.getSelected() == std::optional<std::size_t>(2) && activated.size() == 1,
          "a disabled one it only selects");
    Check(!type(U'z') && list.getSelected() == std::optional<std::size_t>(2), "a key no item has is not taken");

    list.setShortcut(0, U'x');
    Check(type(U'x') && activated.back() == 0 && !type(U'a'), "an item can be given a key of its own");

    list.setItems({"new", "long item, wider than the list"});
    Check(list.getShortcut(0) == U'a' && list.getShortcut(1) == U'b', "a new list is given letters again");

    // Scrolled sideways, the keys stay where they are
    list.Draw(screen, inside);
    list.setScrollOffset(RL::Position(5, 0));
    list.Draw(screen, inside);
    Check(Row(screen, 1).substr(0, 7) == U"b item," && list.getScrollInfo()->content.x == 32,
          "scrolled sideways, the keys stay, and only the items move");

    list.setLetterShortcuts(false);
    list.setItems({"plain"});
    Check(list.getShortcut(0) == 0, "no longer given letters, a new list has no keys");
}

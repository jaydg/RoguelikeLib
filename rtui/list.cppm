// A selectable, scrolling list for menus such as a generator browser. Its
// items are literal text, or, if it is told so, text with colour markup as
// the screen prints it (see rtui.screen). An item may be disabled: it shows
// dimmed, and can be selected, to look at it, but not activated.
module;

export module rtui.list;

export import rtui.windows;
import std;

export namespace RL
{

class CListWindow : public CWindow
{
private:
    std::vector<std::string> items;
    std::optional<std::size_t> selected;
    std::size_t first = 0;
    std::size_t horizontal = 0;
    Size view{0, 0};
    std::function<void(std::size_t)> on_activate;

    // Who follows the selection, told of each item selected
    std::function<void(std::size_t)> on_select;

    // Whether the items are text with colour markup, rather than literal
    // text; and the widest of them, as drawn last
    bool markup = false;
    std::size_t widest = 0;

    // Which items are disabled, by index
    std::vector<bool> disabled;

    // How bright a disabled item shows, per cent of its colours
    static constexpr int disabled_brightness = 50;

    // An item as markup the screen prints: as it is, or, literal, with its
    // '<' written as "<<"
    [[nodiscard]] std::string Markup(std::string_view label) const
    {
        if (markup) {
            return std::string(label);
        }

        std::string text;

        for (const char c : label) {
            text += c == '<' ? std::string("<<") : std::string(1, c);
        }

        return text;
    }

    // Tells whoever follows the selection of the item selected now, if it
    // is another than before - or, for a new list, whichever it is
    void Followed(std::optional<std::size_t> before, bool new_items = false)
    {
        if (selected && (new_items || selected != before) && on_select) {
            // The callback may replace items or the callback itself.
            const auto callback = on_select;
            callback(*selected);
        }
    }

    void KeepSelectionVisible()
    {
        if (!selected || view.y == 0) {
            first = 0;
            return;
        }

        if (*selected < first) {
            first = *selected;
        } else if (*selected - first >= view.y) {
            first = *selected - view.y + 1;
        }

        first = std::min(first, items.size() > view.y ? items.size() - view.y : 0);
    }

    void Move(bool down, std::size_t amount)
    {
        if (!selected) {
            return;
        }

        Select(down ? *selected + std::min(amount, items.size() - 1 - *selected) :
               *selected - std::min(amount, *selected));
    }

public:
    // Labels are literal text, unless told to be markup. The first item is
    // selected initially; an empty list has no selection.
    CListWindow(std::string title, const SRect& floating, std::vector<std::string> labels = {},
                EWindowOptions options = EWindowOptions::None)
        : CWindow(std::move(title), floating, options)
    {
        setItems(std::move(labels));
    }

    // Replacing the list resets selection and scrolling to the first item.
    void setItems(std::vector<std::string> labels)
    {
        items = std::move(labels);
        disabled.assign(items.size(), false);
        selected = items.empty() ? std::nullopt : std::optional<std::size_t> {0};
        first = 0;
        horizontal = 0;
        Followed(std::nullopt, true);
    }

    [[nodiscard]] const std::vector<std::string>& getItems() const
    {
        return items;
    }

    // Disables an item, or enables it again; one that is not there is left
    // as it is. A new list of items has them all enabled.
    void setEnabled(std::size_t index, bool enabled)
    {
        if (index < disabled.size()) {
            disabled[index] = !enabled;
        }
    }

    [[nodiscard]] bool isEnabled(std::size_t index) const
    {
        return index < disabled.size() && !disabled[index];
    }

    // Whether the items are text with colour markup - "<#ffd24a>", or the
    // name of a tag the screen knows - rather than literal text
    void setMarkup(bool a_markup)
    {
        markup = a_markup;
    }

    [[nodiscard]] bool getMarkup() const
    {
        return markup;
    }

    [[nodiscard]] std::optional<std::size_t> getSelected() const
    {
        return selected;
    }

    // Invalid indices leave the selection unchanged.
    bool Select(std::size_t index)
    {
        if (index >= items.size()) {
            return false;
        }

        const auto before = selected;
        selected = index;
        KeepSelectionVisible();
        Followed(before);
        return true;
    }

    // Follows the selection: the callback is told of every item selected,
    // by the keys, the wheel, a click, dragging the scroll bar or Select(),
    // and of the first of every new list of items - to show what it is,
    // say, beside the list
    void setOnSelect(std::function<void(std::size_t)> callback)
    {
        on_select = std::move(callback);
    }

    // Enter or a left click activates the selected item. Navigation only
    // changes selection, allowing a caller to preview it independently.
    void setOnActivate(std::function<void(std::size_t)> callback)
    {
        on_activate = std::move(callback);
    }

    // Activates the selected item, unless it is disabled; returns whether
    // it did
    bool Activate()
    {
        if (!selected || !on_activate || !isEnabled(*selected)) {
            return false;
        }

        // The callback may replace items or the callback itself.
        const auto callback = on_activate;
        const std::size_t index = *selected;
        callback(index);
        return true;
    }

    void Draw(CScreen& screen, const SRect& inside) override
    {
        view = inside.size;
        widest = 0;

        for (const auto& item : items) {
            widest = std::max(widest, screen.TextWidth(Markup(item)));
        }

        KeepSelectionVisible();
        setScrollOffset(Position(horizontal, first));

        for (std::size_t row = 0; row < view.y && row < items.size() - first; ++row) {
            const std::size_t index = first + row;
            const bool inverse = selected == index;

            // Fill the row so the highlight extends through trailing blanks.
            for (std::size_t x = 0; x < view.x; ++x) {
                screen.Put(inside.corner.x + x, inside.corner.y + row, U' ', screen.getTextColour(), false, inverse);
            }

            const std::size_t end = screen.Print(inside.corner.x, inside.corner.y + row, Markup(items[index]), horizontal,
                                                 screen.getTextColour(), false, inverse);

            // A disabled item dimmed, as far as it was drawn
            if (!isEnabled(index) && inside.corner.y + row < screen.getSize().y) {
                for (std::size_t x = inside.corner.x; x < std::min({end, inside.corner.x + view.x, screen.getSize().x}); ++x) {
                    const SCell cell = screen.At(x, inside.corner.y + row);
                    screen.Put(x, inside.corner.y + row, cell.glyph, Dim(cell.rgb, disabled_brightness), cell.bold, cell.inverse);
                }
            }
        }
    }

    [[nodiscard]] std::optional<SScrollInfo> getScrollInfo() const override
    {
        return SScrollInfo{Size(widest, items.size()), view, Position(horizontal, first)};
    }

    [[nodiscard]] bool supportsScrollDragging() const override
    {
        return true;
    }

    void setScrollOffset(Position offset) override
    {
        const auto info = getScrollInfo().value();
        horizontal = std::min(offset.x, info.content.x > view.x ? info.content.x - view.x : 0);
        first = std::min(offset.y, items.size() > view.y ? items.size() - view.y : 0);

        if (selected && view.y > 0) {
            const auto before = selected;
            selected = std::clamp(*selected, first, first + std::min(view.y, items.size() - first) - 1);
            Followed(before);
        }
    }

    bool HandleKey(const SKey& key) override
    {
        if (!selected) {
            return false;
        }

        switch (key.type) {
        case EKey::Up:
            Move(false, 1);
            return true;

        case EKey::Down:
            Move(true, 1);
            return true;

        case EKey::Home:
            Select(0);
            return true;

        case EKey::End:
            Select(items.size() - 1);
            return true;

        case EKey::PageUp:
            Move(false, std::max<std::size_t>(view.y, 1));
            return true;

        case EKey::PageDown:
            Move(true, std::max<std::size_t>(view.y, 1));
            return true;

        case EKey::Enter:
            return Activate();

        default:
            return false;
        }
    }

    void Scroll(int lines) override
    {
        const std::int64_t distance = lines;
        Move(distance > 0, static_cast<std::size_t>(distance < 0 ? -distance : distance));
    }

    void Click(Position at, EMouseButton button) override
    {
        if (button == EMouseButton::Left && at.x < view.x && at.y < view.y && at.y < items.size() - first) {
            Select(first + at.y);
            Activate();
        }
    }
};

} // namespace RL

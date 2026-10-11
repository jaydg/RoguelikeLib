// A selectable, scrolling list for menus such as a generator browser.
module;

export module rtui.list;

export import rtui.windows;
import rtui.utf8;
import std;

export namespace RL
{

class CListWindow : public CWindow
{
private:
    std::vector<std::string> items;
    std::optional<std::size_t> selected;
    std::size_t first = 0;
    Size view{0, 0};
    std::function<void(std::size_t)> on_activate;

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
    // Labels are literal text, not colour markup. The first item is
    // selected initially; an empty list has no selection.
    CListWindow(std::string title, const SRect& floating, std::vector<std::string> labels = {})
        : CWindow(std::move(title), floating)
    {
        setItems(std::move(labels));
    }

    // Replacing the list resets selection and scrolling to the first item.
    void setItems(std::vector<std::string> labels)
    {
        items = std::move(labels);
        selected = items.empty() ? std::nullopt : std::optional<std::size_t> {0};
        first = 0;
    }

    [[nodiscard]] const std::vector<std::string>& getItems() const
    {
        return items;
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

        selected = index;
        KeepSelectionVisible();
        return true;
    }

    // Enter or a left click activates the selected item. Navigation only
    // changes selection, allowing a caller to preview it independently.
    void setOnActivate(std::function<void(std::size_t)> callback)
    {
        on_activate = std::move(callback);
    }

    bool Activate()
    {
        if (!selected || !on_activate) {
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
        KeepSelectionVisible();

        for (std::size_t row = 0; row < view.y && row < items.size() - first; ++row) {
            const std::size_t index = first + row;
            const bool inverse = selected == index;

            // Fill the row so the highlight extends through trailing blanks.
            for (std::size_t x = 0; x < view.x; ++x) {
                screen.Put(inside.corner.x + x, inside.corner.y + row, U' ', screen.getTextColour(), false, inverse);
            }

            std::string_view label = items[index];

            for (std::size_t x = 0; x < view.x && !label.empty(); ++x) {
                const auto decoded = DecodeUTF8(label);
                screen.Put(inside.corner.x + x, inside.corner.y + row, decoded.code_point,
                           screen.getTextColour(), false, inverse);
                label.remove_prefix(decoded.length);
            }

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

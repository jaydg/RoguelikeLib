import rl.calendar;
import rl.clock;
import rl.distancemap;
import rl.fov;
import rl.mapgenerators;
import rl.pathfinding;
import rl.randomness;
import rl.scheduler;
import rl.tile;
import rtui.list;
import rtui.map;
import std;

using namespace RL;

struct SGenerator {
    std::string name;
    std::function<void(CMap&)> generate;
};

std::vector<SGenerator> Generators()
{
    std::vector<SGenerator> result = {
        {
            "Standard Dungeon", [](CMap & map)
            {
                CreateStandardDungeon(map, 20);
            }
        },
        {
            "Ant's Nest", [](CMap & map)
            {
                CreateAntNest(map, true);
            }
        },
        {
            "Mines", [](CMap & map)
            {
                CreateMines(map, 25);
            }
        },
        {
            "Caves Sharp", [](CMap & map)
            {
                CreateCaves(map, 1, 0.75);
            }
        },
        {
            "Caves Soft", [](CMap & map)
            {
                CreateCaves(map, 3);
            }
        },
        {
            "Maze", [](CMap & map)
            {
                CreateMaze(map);
            }
        },
        {
            "Space shuttle", [](CMap & map)
            {
                CreateSpaceShuttle(map, 25);
            }
        },
        {
            "Forest", [](CMap & map)
            {
                GenerateForest(map);
            }
        },
        {
            "Cave and Glade - Cave", [](CMap & map)
            {
                CreateCaveAndGlade(map, "wall", "room", CaveShape, CaveWater);
            }
        },
        {
            "Small cave", [](CMap & map)
            {
                CreateCaveAndGlade(map, "wall", "room", {.min_areas = 7, .max_areas = 11, .scale = 2}, {.shallow = {}});
            }
        },
        {
            "Cave and Glade - Glade", [](CMap & map)
            {
                CreateCaveAndGlade(map, "tree", "grass", GladeShape, GladeWater);
            }
        },
        {
            "Unicode glade", [](CMap & map)
            {
                CreateCaveAndGlade(map, "pine", "grass", GladeShape, {.shallow = "pond", .level = 12, .deep_level = 35, .grain = 20});
            }
        },
        {
            "Voronoi Wilderness", [](CMap & map)
            {
                CreateVoronoiWilderness(map, 24, true);
            }
        },
        {
            "Simple City", [](CMap & map)
            {
                CreateSimpleCity(map, 15);
            }
        }
    };

    for (const auto& name : GetDelvePresets()) {
        result.push_back({"Delve - " + std::string(name), [preset = std::string(name)](CMap & map)
        {
            CreateDelve(map, preset);
        }});
    }

    return result;
}

// Each tab owns its demonstration state. The generator tab embeds the
// library list widget, forwarding input in its sidebar to that widget.
class CFeatureWindow : public CWindow
{
private:
    int mode;
    std::vector<SGenerator> generators;
    CListWindow list{"Generators", {Position(0, 0), Size(30, 10)}};
    CMap level{Size(79, 50)};
    bool ready = false;
    std::size_t generated = 0;
    Position focus{0, 0};
    Position origin{0, 0};
    Position start{0, 0};
    Position goal{0, 0};
    Position content_origin{0, 0};
    SRect map_area;
    SRect list_area;
    std::vector<Position> path;
    std::string detail;
    CScheduler<std::string> scheduler;
    std::vector<std::string> history;
    CClock clock{30};
    CCalendar calendar{{{"Thawmoon", 30}, {"Seedmoon", 30}, {"Sunmoon", 30}, {"Frostmoon", 30}}};
    CDailySchedule<std::string> phases;

    Position FirstFloor(bool last = false) const
    {
        Position found{0, 0};

        for (std::size_t y = 0; y < level.getHeight(); ++y) {
            for (std::size_t x = 0; x < level.getWidth(); ++x) {
                if (level.get(x, y).isPassable()) {
                    found = Position(x, y);

                    if (!last) {
                        return found;
                    }
                }
            }
        }

        return found;
    }

    void Generate()
    {
        level.Clear();
        path.clear();

        if (mode == 0) {
            generated = list.getSelected().value_or(0);
            generators[generated].generate(level);
            detail = generators[generated].name;
            focus = Position(level.getWidth() / 2, level.getHeight() / 2);
        } else if (mode == 1) {
            CreateSimpleCity(level, 15);
            focus = FirstFloor();
            detail = "Arrows move observer; visible tiles are bright";
        } else if (mode == 2) {
            CreateMaze(level);
            start = FirstFloor();
            goal = FirstFloor(true);
            const bool found = FindPath(level, start, goal, path, false);
            focus = start;
            detail = found ? std::format("Path: {} steps; S start / E end / + trail", path.size()) : "No path found";
        } else if (mode == 3) {
            CreateCaves(level, 3);
            goal = FirstFloor();
            focus = goal;
            detail = "Click a floor to set goal; digits show distance to @";
        }

        ready = true;
    }

    void Step()
    {
        if (mode == 4) {
            if (const auto actor = scheduler.Next()) {
                history.push_back(std::format("Tick {:4}: {} acts", scheduler.Now(), *actor));
                scheduler.Schedule(*actor, *actor == "hare" ? 50 : *actor == "fox" ? 100 : 400);

                if (history.size() > 100) {
                    history.erase(history.begin());
                }
            }
        } else if (mode == 5) {
            clock.Advance();
        }
    }

    void Move(int dx, int dy)
    {
        const auto x = static_cast<std::int64_t>(focus.x) + dx;
        const auto y = static_cast<std::int64_t>(focus.y) + dy;

        if (x < 0 || y < 0 || x >= static_cast<std::int64_t>(level.getWidth()) || y >= static_cast<std::int64_t>(level.getHeight())) {
            return;
        }

        const Position next(static_cast<std::size_t>(x), static_cast<std::size_t>(y));

        if (mode != 1 || level.get(next).isPassable()) {
            focus = next;
        }
    }

public:
    CFeatureWindow(std::string title, int a_mode)
        : CWindow(std::move(title),
    {
        Position(0, 0), Size(100, 30)
    }), mode(a_mode)
    {
        if (mode == 0) {
            generators = Generators();
            std::vector<std::string> labels;

            for (const auto& generator : generators) {
                labels.push_back(generator.name);
            }

            list.setItems(std::move(labels));
            list.setOnActivate([this](std::size_t) {
                Generate();
            });
        }

        if (mode == 4) {
            for (const auto& actor : {
                        "hare", "fox", "snail"
                    }) {
                scheduler.Schedule(actor, 0);
            }
        }

        phases.Set(6, 0, "morning");
        phases.Set(12, 0, "afternoon");
        phases.Set(18, 0, "evening");
        phases.Set(22, 0, "night");
    }

    void Draw(CScreen& screen, const SRect& inside) override
    {
        content_origin = inside.corner;

        if (inside.Empty()) {
            return;
        }

        if (mode >= 4) {
            if (mode == 4) {
                screen.Print(inside.corner.x, inside.corner.y, "Enter/Space: next actor | hare 50, fox 100, snail 400 ticks");
                const std::size_t rows = inside.size.y - 1;
                const std::size_t first = history.size() > rows ? history.size() - rows : 0;

                for (std::size_t index = first; index < history.size(); ++index) {
                    screen.Print(inside.corner.x, inside.corner.y + 1 + index - first, history[index]);
                }
            } else {
                const SDate date = calendar.DateOf(clock.Day());
                const std::vector<std::string> lines = {
                    "Enter/Space: advance 30 minutes | D: advance one day",
                    std::format("Year {}, {} of {}", date.year, Ordinal(date.day), calendar.PartName(date.part)),
                    std::format("Time: {} ({})", clock.ToString(), phases.At(clock)),
                    std::format("Elapsed: {} days / {} minutes", clock.Day(), clock.Minutes()),
                    std::format("Calendar: {} months, {} days per year", calendar.Parts(), calendar.DaysPerYear())
                };

                for (std::size_t row = 0; row < std::min(lines.size(), inside.size.y); ++row) {
                    screen.Print(inside.corner.x, inside.corner.y + row, lines[row]);
                }
            }

            return;
        }

        if (!ready) {
            Generate();
        }

        screen.Print(inside.corner.x, inside.corner.y, detail);
        map_area = {Position(inside.corner.x, inside.corner.y + 1), Size(inside.size.x, inside.size.y - 1)};
        list_area = {};

        if (mode == 0) {
            const std::size_t width = std::min<std::size_t>(30, inside.size.x / 2);
            list_area = {map_area.corner, Size(width, map_area.size.y)};
            list.Draw(screen, list_area);
            map_area.corner.x += width + 1;
            map_area.size.x -= std::min(width + 1, map_area.size.x);
        }

        origin = CameraOrigin(level.getSize(), map_area.size, focus);
        DrawMap(screen, level, map_area, origin);

        if (mode == 1 || mode == 3) {
            CFOV fov(&level);
            fov.Calculate(focus, 9);
            const CDistanceMap distances(level, std::array{goal});

            for (std::size_t y = 0; y < map_area.size.y && origin.y + y < level.getHeight(); ++y) {
                for (std::size_t x = 0; x < map_area.size.x && origin.x + x < level.getWidth(); ++x) {
                    const Position pos(origin.x + x, origin.y + y);
                    const auto& tile = level.get(pos);
                    char32_t glyph = tile.getGlyph();
                    std::uint32_t colour = tile.getColor();

                    if (mode == 1) {
                        colour = fov.get(pos) ? colour : Dim(colour, 15);

                        if (pos == focus) {
                            glyph = U'@';
                            colour = 0xFFFF00;
                        }
                    } else {
                        const int distance = distances.Distance(pos);
                        glyph = distance == CDistanceMap::unreachable ? (tile.isPassable() ? U' ' : U'#') : U'0' + distance % 10;
                        colour = distance == CDistanceMap::unreachable ? 0x404040 : Blend(0xFFFF00, 0x4080FF, std::min(distance, 60) * 100 / 60);

                        if (pos == goal) {
                            glyph = U'@';
                        }
                    }

                    screen.Put(map_area.corner.x + x, map_area.corner.y + y, glyph, colour);
                }
            }
        } else if (mode == 2) {
            for (const auto& pos : path) {
                if (const auto at = OnScreen(map_area, origin, pos)) {
                    screen.Put(at->x, at->y, U'+', 0xFFFF00);
                }
            }

            for (const auto& [pos, glyph] : {
                        std::pair{start, U'S'}, std::pair{goal, U'E'}
                    }) {
                if (const auto at = OnScreen(map_area, origin, pos)) {
                    screen.Put(at->x, at->y, glyph, 0xFFFFFF);
                }
            }
        }
    }

    bool HandleKey(const SKey& key) override
    {
        if (mode == 0 && list.HandleKey(key)) {
            if (list.getSelected() != generated) {
                Generate();
            }

            return true;
        }

        if (key.type == EKey::Char && (key.ch == U'r' || key.ch == U'R') && mode < 4) {
            Generate();
            return true;
        }

        if (mode >= 4 && (key.type == EKey::Enter || (key.type == EKey::Char && key.ch == U' '))) {
            Step();
            return true;
        }

        if (mode == 5 && key.type == EKey::Char && (key.ch == U'd' || key.ch == U'D')) {
            clock.Advance(48);
            return true;
        }

        if (mode < 4) {
            if (!ready) {
                Generate();
            }

            if (key.type == EKey::Left || (key.type == EKey::Char && key.ch == U'h')) {
                Move(-1, 0);
            } else if (key.type == EKey::Right || (key.type == EKey::Char && key.ch == U'l')) {
                Move(1, 0);
            } else if (key.type == EKey::Up || (key.type == EKey::Char && key.ch == U'k')) {
                Move(0, -1);
            } else if (key.type == EKey::Down || (key.type == EKey::Char && key.ch == U'j')) {
                Move(0, 1);
            } else {
                return false;
            }

            return true;
        }

        return false;
    }

    void Click(Position at, EMouseButton button) override
    {
        if (button != EMouseButton::Left) {
            return;
        }

        const Position absolute(content_origin.x + at.x, content_origin.y + at.y);

        if (mode == 0 && list_area.Contains(absolute)) {
            list.Click(Position(absolute.x - list_area.corner.x, absolute.y - list_area.corner.y), button);
        } else if ((mode == 1 || mode == 3) && map_area.Contains(absolute)) {
            const Position pos(origin.x + absolute.x - map_area.corner.x, origin.y + absolute.y - map_area.corner.y);

            if (level.inside(pos) && level.get(pos).isPassable()) {
                if (mode == 1) {
                    focus = pos;
                } else {
                    goal = pos;
                }
            }
        }
    }

    void Scroll(int lines) override
    {
        if (mode == 0) {
            list.Scroll(lines);
            Generate();
        } else if (mode < 4) {
            Move(0, lines);
        }
    }
};

int main(int argc, char* argv[])
{
    try {
        const std::uint32_t seed = argc > 1 ? static_cast<std::uint32_t>(std::stoul(argv[1])) : InitRandomness();
        InitRandomness(seed);
        CTileData::RegisterTile("pine", {U'♣', 0x2E7D32, false, false});
        CTileData::RegisterTile("pond", {U'≈', 0x3399FF, true, false});
        CTerminal terminal;
        CScreen screen;
        CWindowManager windows;
        windows.setAllowTabUndocking(false);
        std::vector<std::unique_ptr<CFeatureWindow>> tabs;
        const std::array titles = {"Map generators", "FOV", "Path finding", "Distance map", "Scheduler", "Calendar"};

        for (std::size_t index = 0; index < titles.size(); ++index) {
            tabs.push_back(std::make_unique<CFeatureWindow>(titles[index], static_cast<int>(index)));
            windows.Open(*tabs.back());
            windows.Dock(*tabs.back(), EDock::Top, 1000);

            if (index > 0) {
                windows.Tab(*tabs.back(), *tabs.front());
            }
        }

        windows.Focus(*tabs.front());
        std::size_t active = 0;

        for (;;) {
            screen.Resize(terminal.getSize());
            screen.Clear();
            const Size size = screen.getSize();

            if (size.x < 40 || size.y < 10) {
                screen.Print(0, 0, "Resize to at least 40x10; Q quits");
            } else {
                windows.Layout({Position(0, 0), Size(size.x, size.y - 1)});
                windows.Draw(screen);
                screen.Print(0, size.y - 1, "1-6/Tab: tabs | R: regenerate | hjkl: pan | Q: quit");
            }

            terminal.Write(screen.Compose(false));
            const SKey key = terminal.ReadKey();

            if (key.type == EKey::Closed || key.type == EKey::Escape ||
                    (key.type == EKey::Char && (key.ch == U'q' || key.ch == U'Q' || key.ch == 3))) {
                break;
            }

            if (key.type == EKey::Mouse && size.x >= 40 && size.y >= 10) {
                windows.HandleMouse(key.mouse);

                for (std::size_t index = 0; index < tabs.size(); ++index) {
                    if (windows.getFocused() == tabs[index].get()) {
                        active = index;
                    }
                }
            } else if (key.type == EKey::Char && (key.ch == U'\t' || (key.ch >= U'1' && key.ch <= U'6'))) {
                active = key.ch == U'\t' ? (active + 1) % tabs.size() : static_cast<std::size_t>(key.ch - U'1');
                windows.Open(*tabs[active]);
            } else if (size.x >= 40 && size.y >= 10) {
                windows.HandleKey(key);
            }
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Feature demo: " << error.what() << '\n';
        return 1;
    }
}

import rl.calendar;
import rl.clock;
import rl.distancemap;
import rl.fov;
import rl.mapgenerators;
import rl.maputils;
import rl.pathfinding;
import rl.randomness;
import rl.scheduler;
import rl.tile;
import rtui.list;
import rtui.input;
import rtui.map;
import std;

using namespace RL;

struct SGenerator {
    std::string name;
    std::function<void(CMap&)> generate;
    std::function<std::string()> variant{};
};

struct CCaveGladeConfig {
    struct BiomeEntry {
        std::string label;
        std::function<TerrainBiome()> create;
    };

    struct ShapeEntry {
        std::string label;
        SCaveAndGladeShape shape;
    };

    struct WaterEntry {
        std::string label;
        SCaveAndGladeWater water;
    };

    struct PresetEntry {
        std::string label;
        std::size_t wall;
        std::size_t floor;
        std::size_t shape;
        std::size_t water;
    };

    std::vector<BiomeEntry> walls = {
        {"Forest Pine (Unicode)", []() -> TerrainBiome {
            return SForestBiome{.vegetation_density = 0.85f, .tree_tile = "pine"};
        }},
        {"Forest (ASCII)", []() -> TerrainBiome {
            return SForestBiome{.vegetation_density = 0.85f};
        }},
        {"Solid Wall", []() -> TerrainBiome {
            return SSolidBiome{"wall"};
        }},
        {"Solid Trees", []() -> TerrainBiome {
            return SSolidBiome{"tree"};
        }},
        {"Solid Pine (Unicode)", []() -> TerrainBiome {
            return SSolidBiome{"pine"};
        }},
        {"Rolling Hills", []() -> TerrainBiome {
            return SHillsBiome{};
        }},
        {"Bogland", []() -> TerrainBiome {
            return SBogBiome{};
        }}
    };

    std::vector<BiomeEntry> floors = {
        {"Grassland Pine (Unicode)", []() -> TerrainBiome {
            return SGrasslandBiome{.tree_tile = "pine"};
        }},
        {"Grassland (ASCII)", []() -> TerrainBiome {
            return SGrasslandBiome{};
        }},
        {"Dungeon Room", []() -> TerrainBiome {
            return SSolidBiome{"room"};
        }},
        {"Plain Grass", []() -> TerrainBiome {
            return SSolidBiome{"grass"};
        }},
        {"Bogland", []() -> TerrainBiome {
            return SBogBiome{};
        }},
        {"Rolling Hills", []() -> TerrainBiome {
            return SHillsBiome{};
        }}
    };

    std::vector<ShapeEntry> shapes = {
        {"Glade (Looping)", GladeShape},
        {"Cave (Standard)", CaveShape},
        {"Small Cave", SCaveAndGladeShape{.min_areas = 7, .max_areas = 11, .scale = 2}},
        {"Grand Cavern", SCaveAndGladeShape{.min_areas = 6, .max_areas = 10, .scale = 4, .loop_odds = 35}}
    };

    std::vector<WaterEntry> waters = {
        {"Unicode Ponds", SCaveAndGladeWater{.shallow = "pond", .level = 12, .deep_level = 35, .grain = 20}},
        {"Glade Ponds (ASCII)", GladeWater},
        {"Cave Pools", CaveWater},
        {"None (Dry)", SCaveAndGladeWater{.shallow = {}}},
        {"Flooded Swamp", SCaveAndGladeWater{.shallow = "water", .level = 28, .deep_level = 50, .grain = 16}}
    };

    std::vector<PresetEntry> presets = {
        {"Unicode Glade", 0, 0, 0, 0},
        {"Cave and Glade - Glade", 3, 3, 0, 1},
        {"Cave and Glade - Cave", 2, 2, 1, 2},
        {"Small Cave", 2, 2, 2, 3},
        {"Bog Cavern", 2, 4, 1, 3},
        {"Mountain Meadow", 5, 0, 0, 0}
    };

    std::size_t wall_idx = 0;
    std::size_t floor_idx = 0;
    std::size_t shape_idx = 0;
    std::size_t water_idx = 0;
    std::optional<std::size_t> preset_idx = 0;

    void CheckPresetMatch()
    {
        preset_idx.reset();
        for (std::size_t i = 0; i < presets.size(); ++i) {
            if (presets[i].wall == wall_idx &&
                presets[i].floor == floor_idx &&
                presets[i].shape == shape_idx &&
                presets[i].water == water_idx) {
                preset_idx = i;
                break;
            }
        }
    }

    void ApplyPreset(std::size_t idx)
    {
        if (idx < presets.size()) {
            preset_idx = idx;
            wall_idx = presets[idx].wall;
            floor_idx = presets[idx].floor;
            shape_idx = presets[idx].shape;
            water_idx = presets[idx].water;
        }
    }

    void Generate(CMap& map) const
    {
        RegisterBiomeTiles();
        CreateCaveAndGlade(map,
                           walls[wall_idx].create(),
                           floors[floor_idx].create(),
                           shapes[shape_idx].shape,
                           waters[water_idx].water);
    }

    [[nodiscard]] std::string Summary() const
    {
        if (preset_idx) {
            return std::format("{} | C: configure", presets[*preset_idx].label);
        }
        return std::format("{} / {} | C: configure", walls[wall_idx].label, floors[floor_idx].label);
    }
};

std::vector<SGenerator> Generators(std::shared_ptr<CCaveGladeConfig> cave_glade_config = nullptr)
{
    const auto voronoi_buildings = std::make_shared<bool>(false);
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
            "Riverland", [](CMap & map)
            {
                CreateRiverland(map);
            }
        },
        {
            "Cave and Glade", [cave_glade_config](CMap & map)
            {
                if (cave_glade_config) {
                    cave_glade_config->Generate(map);
                } else {
                    CreateCaveAndGlade(map, "wall", "room", CaveShape, CaveWater);
                }
            },
            [cave_glade_config]()
            {
                return cave_glade_config ? cave_glade_config->Summary() : "C: configure";
            }
        },
        {
            "Multi-Region Wilderness", [voronoi_buildings](CMap & map)
            {
                *voronoi_buildings = Random(2) != 0;
                CreateMultiRegion(map, 0, *voronoi_buildings);
            },
            [voronoi_buildings]()
            {
                return *voronoi_buildings ? "buildings and fields" : "no buildings; bogs";
            }
        },
        {
            "Growth Town", [](CMap & map)
            {
                CreateGrowthTown(map);
            }
        },
        {
            "Town in Wilderness", [](CMap & map)
            {
                CreateRiverland(map);
                CMap town(Size(map.getWidth() * 2 / 3, map.getHeight() * 2 / 3));
                std::vector<Position> gates;
                CreateGrowthTown(town, 80, &gates);
                StampTownWithRoads(map, town, Position((map.getWidth() - town.getWidth()) / 2,
                                                      (map.getHeight() - town.getHeight()) / 2), gates);
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

// The shared input widget supplies editing and submission; this window
// adds the current dimensions, usage hints and validation feedback.
class CMapSizeInput : public CInputWindow
{
    Size current{79, 50};
    std::string message;
public:
    CMapSizeInput()
        : CInputWindow("Map size", {Position(4, 3), Size(32, 6)}, 20,
                       EWindowOptions::NoResize | EWindowOptions::NoMove | EWindowOptions::NoClose)
    {
        setPrompt("New size: ");
    }
    void Begin(Size size)
    {
        current = size;
        message.clear();
        setText("");
    }
    void setMessage(std::string value) { message = std::move(value); }
    void Draw(CScreen& screen, const SRect& inside) override
    {
        if (inside.Empty()) {
            return;
        }
        CInputWindow::Draw(screen, inside);
        if (inside.size.y > 1) {
            screen.Print(inside.corner.x, inside.corner.y + 1, std::format("Current: {}x{}", current.x, current.y));
        }
        if (inside.size.y > 2) {
            screen.Print(inside.corner.x, inside.corner.y + 2,
                         message.empty() ? "Enter: apply | Esc: cancel" : message);
        }
    }
};

// Configuration dialog for Cave and Glade biome combination
class CCaveGladeConfigWindow : public CWindow
{
    std::shared_ptr<CCaveGladeConfig> config;
    std::function<void()> on_change;
    std::size_t selected_row = 0;
    bool done = false;

public:
    CCaveGladeConfigWindow(std::shared_ptr<CCaveGladeConfig> a_config, std::function<void()> a_on_change)
        : CWindow("Cave & Glade Biome Studio", {Position(4, 3), Size(50, 10)},
                  EWindowOptions::NoResize | EWindowOptions::NoMove | EWindowOptions::NoClose),
          config(std::move(a_config)), on_change(std::move(a_on_change))
    {
    }

    [[nodiscard]] bool TakeDone()
    {
        const bool res = done;
        done = false;
        return res;
    }

    void CycleRow(int dir)
    {
        if (!config) return;
        if (selected_row == 0) {
            const int count = static_cast<int>(config->presets.size());
            int cur = config->preset_idx.value_or(0);
            cur = (cur + dir + count) % count;
            config->ApplyPreset(static_cast<std::size_t>(cur));
        } else if (selected_row == 1) {
            const int count = static_cast<int>(config->walls.size());
            config->wall_idx = (static_cast<int>(config->wall_idx) + dir + count) % count;
            config->CheckPresetMatch();
        } else if (selected_row == 2) {
            const int count = static_cast<int>(config->floors.size());
            config->floor_idx = (static_cast<int>(config->floor_idx) + dir + count) % count;
            config->CheckPresetMatch();
        } else if (selected_row == 3) {
            const int count = static_cast<int>(config->shapes.size());
            config->shape_idx = (static_cast<int>(config->shape_idx) + dir + count) % count;
            config->CheckPresetMatch();
        } else if (selected_row == 4) {
            const int count = static_cast<int>(config->waters.size());
            config->water_idx = (static_cast<int>(config->water_idx) + dir + count) % count;
            config->CheckPresetMatch();
        }
        if (on_change) {
            on_change();
        }
    }

    void Draw(CScreen& screen, const SRect& inside) override
    {
        if (inside.Empty() || !config) return;

        const std::string preset_str = config->preset_idx ? config->presets[*config->preset_idx].label : "(Custom)";

        struct RowDisplay {
            std::string label;
            std::string value;
        };

        const std::array<RowDisplay, 5> rows = {{
            {"Preset:", preset_str},
            {"Wall Biome:", config->walls[config->wall_idx].label},
            {"Floor Biome:", config->floors[config->floor_idx].label},
            {"Layout:", config->shapes[config->shape_idx].label},
            {"Water:", config->waters[config->water_idx].label}
        }};

        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (i >= inside.size.y) break;
            const bool is_selected = (i == selected_row);
            const std::string prefix = is_selected ? "> " : "  ";
            const std::uint32_t colour = is_selected ? 0xFFFF70 : 0xC0C0C0;

            const std::string text = std::format("{}{:13} ◄ {:^24} ►",
                                                 prefix, rows[i].label, rows[i].value);
            screen.Print(inside.corner.x + 1, inside.corner.y + i, text, colour, is_selected);
        }

        if (inside.size.y > 6) {
            screen.Print(inside.corner.x + 1, inside.corner.y + 6,
                         "↑/↓: row | ←/→: cycle | R: re-roll | Enter: done",
                         0x808080);
        }
    }

    bool HandleKey(const SKey& key) override
    {
        if (key.type == EKey::Closed || key.type == EKey::Escape ||
            (key.type == EKey::Char && (key.ch == U'q' || key.ch == U'Q' || key.ch == 3))) {
            done = true;
            return true;
        }
        if (key.type == EKey::Enter || (key.type == EKey::Char && (key.ch == U'c' || key.ch == U'C'))) {
            done = true;
            return true;
        }
        if (key.type == EKey::Up || (key.type == EKey::Char && key.ch == U'k')) {
            selected_row = (selected_row + 4) % 5;
            return true;
        }
        if (key.type == EKey::Down || (key.type == EKey::Char && key.ch == U'j')) {
            selected_row = (selected_row + 1) % 5;
            return true;
        }
        if (key.type == EKey::Left || (key.type == EKey::Char && key.ch == U'h')) {
            CycleRow(-1);
            return true;
        }
        if (key.type == EKey::Right || (key.type == EKey::Char && (key.ch == U'l' || key.ch == U' '))) {
            CycleRow(1);
            return true;
        }
        if (key.type == EKey::Char && (key.ch == U'r' || key.ch == U'R')) {
            if (on_change) on_change();
            return true;
        }
        return false;
    }

    void Click(Position at, EMouseButton button) override
    {
        if (button != EMouseButton::Left) return;
        if (at.y < 5) {
            selected_row = at.y;
            if (at.x < 24) {
                CycleRow(-1);
            } else {
                CycleRow(1);
            }
        } else if (at.y >= 6) {
            done = true;
        }
    }
};

// Each tab owns its demonstration state. The generator tab embeds the
// library list widget, forwarding input in its sidebar to that widget.
class CFeatureWindow : public CWindow
{
private:
    int mode;
    std::vector<SGenerator> generators;
    CListWindow list{"Generators", {Position(0, 0), Size(30, 10)}};
    std::optional<Size> custom_size;
    CMap level;
    bool ready = false;
    bool paused = false;
    std::size_t generated = 0;
    Position focus{0, 0};
    Position origin{0, 0};
    std::optional<Position> scroll_origin;
    std::optional<Position> hover_position;
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
    std::function<void()> open_config_cb;

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
            scroll_origin.reset();
        }
    }

public:
    void Generate()
    {
        scroll_origin.reset();
        level.Clear();
        path.clear();

        if (mode == 0) {
            generated = list.getSelected().value_or(0);
            generators[generated].generate(level);
            detail = std::format("{} ({}x{})", generators[generated].name, level.getWidth(), level.getHeight());
            if (generators[generated].variant) {
                detail += ": " + generators[generated].variant();
            }
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

    bool SelectGenerator(std::string_view name)
    {
        if (mode != 0) {
            return false;
        }
        for (std::size_t i = 0; i < generators.size(); ++i) {
            if (generators[i].name == name) {
                list.Select(i);
                Generate();
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] Size getMapSize() const { return level.getSize(); }

    void Hover(std::optional<Position> position) { hover_position = position; }

    void setMapSize(Size size)
    {
        custom_size = size;
        ready = false;
        scroll_origin.reset();
    }

    [[nodiscard]] bool IsRunning() const
    {
        return mode >= 4 && !paused;
    }

    void Tick()
    {
        if (IsRunning()) {
            Step();
        }
    }

    CFeatureWindow(std::string title, int a_mode, std::optional<Size> map_size = {},
                   std::shared_ptr<CCaveGladeConfig> a_cave_glade_config = nullptr,
                   std::function<void()> a_open_config = nullptr)
        : CWindow(std::move(title),
    {
        Position(0, 0), Size(100, 30)
    }), mode(a_mode), custom_size(map_size), level(map_size.value_or(Size(79, 50))),
      open_config_cb(std::move(a_open_config))
    {
        if (mode == 0) {
            generators = Generators(a_cave_glade_config);
            std::vector<std::string> labels;

            for (const auto& generator : generators) {
                labels.push_back(generator.name);
            }

            list.setItems(std::move(labels));
            list.setOnActivate([this](std::size_t index) {
                if (generators[index].name == "Cave and Glade" && open_config_cb) {
                    open_config_cb();
                } else {
                    Generate();
                }
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
                screen.Print(inside.corner.x, inside.corner.y, std::format("{} | Enter/Space: {} | one actor/second", paused ? "Paused" : "Running", paused ? "continue" : "pause"));
                const std::size_t rows = inside.size.y - 1;
                const std::size_t first = history.size() > rows ? history.size() - rows : 0;

                for (std::size_t index = first; index < history.size(); ++index) {
                    screen.Print(inside.corner.x, inside.corner.y + 1 + index - first, history[index]);
                }
            } else {
                const SDate date = calendar.DateOf(clock.Day());
                const std::vector<std::string> lines = {
                    std::format("{} | Enter/Space: {} | 30 minutes/second | D: next day", paused ? "Paused" : "Running", paused ? "continue" : "pause"),
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

        map_area = {Position(inside.corner.x, inside.corner.y + 1), Size(inside.size.x, inside.size.y - 1)};
        list_area = {};

        if (mode == 0) {
            const std::size_t width = std::min<std::size_t>(30, inside.size.x / 2);
            list_area = {map_area.corner, Size(width, map_area.size.y)};
            list.Draw(screen, list_area);
            map_area.corner.x += width + 1;
            map_area.size.x -= std::min(width + 1, map_area.size.x);
        }

        // Keep the default map on small terminals; wide viewports use all
        // available columns. A changed width needs a newly generated map.
        const Size map_size = custom_size.value_or(Size(std::max<std::size_t>(79, map_area.size.x), 50));

        if (level.getSize() != map_size) {
            level = CMap(map_size);
            ready = false;
        }

        if (!ready) {
            Generate();
        }

        origin = scroll_origin.value_or(CameraOrigin(level.getSize(), map_area.size, focus));
        origin.x = std::min(origin.x, level.getWidth() > map_area.size.x ? level.getWidth() - map_area.size.x : 0);
        origin.y = std::min(origin.y, level.getHeight() > map_area.size.y ? level.getHeight() - map_area.size.y : 0);
        std::string status = detail;
        if (mode == 0 && hover_position && map_area.Contains(*hover_position)) {
            const Position tile_position(origin.x + hover_position->x - map_area.corner.x,
                                         origin.y + hover_position->y - map_area.corner.y);
            if (level.inside(tile_position)) {
                std::string description(level.get(tile_position).getType());
                if (description == "door_closed") description = "closed door";
                else if (description == "door_open") description = "open door";
                else if (description == "room") description = "floor";
                else std::replace(description.begin(), description.end(), '_', ' ');
                status += " | " + description;
            }
        }
        screen.Print(inside.corner.x, inside.corner.y, status);
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

        if (mode == 0 && key.type == EKey::Char && (key.ch == U'c' || key.ch == U'C')) {
            if (open_config_cb) {
                open_config_cb();
                return true;
            }
        }

        if (key.type == EKey::Char && (key.ch == U'r' || key.ch == U'R') && mode < 4) {
            Generate();
            return true;
        }

        if (mode >= 4 && (key.type == EKey::Enter || (key.type == EKey::Char && key.ch == U' '))) {
            paused = !paused;
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
                    scroll_origin.reset();
                } else {
                    goal = pos;
                }
            }
        }
    }

    [[nodiscard]] std::optional<SScrollInfo> getScrollInfo() const override
    {
        if (mode < 4 && ready) {
            return SScrollInfo{level.getSize(), map_area.size, origin};
        }

        return std::nullopt;
    }

    [[nodiscard]] bool supportsScrollDragging() const override
    {
        return mode < 4;
    }

    void setScrollOffset(Position offset) override
    {
        scroll_origin = offset;
        origin = offset;
    }

    void Scroll(int lines) override
    {
        if (mode == 0) {
            const std::size_t maximum = level.getHeight() > map_area.size.y ? level.getHeight() - map_area.size.y : 0;
            const auto y = std::clamp(static_cast<std::int64_t>(origin.y) + lines,
                                      std::int64_t{0}, static_cast<std::int64_t>(maximum));
            setScrollOffset(Position(origin.x, static_cast<std::size_t>(y)));
        } else if (mode < 4) {
            Move(0, lines);
        }
    }
};

int main(int argc, char* argv[])
{
    try {
        std::optional<std::uint32_t> requested_seed;
        std::optional<Size> map_size;
        auto number = [](std::string_view text) {
            std::size_t value = 0;
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (error != std::errc{} || end != text.data() + text.size()) {
                throw std::invalid_argument("Expected an unsigned integer: " + std::string(text));
            }
            return value;
        };
        for (int i = 1; i < argc; ++i) {
            const std::string_view argument(argv[i]);
            if (argument == "--help" || argument == "-h") {
                std::cout << "Usage: demo-features [SEED] [--size WIDTHxHEIGHT]\n"
                          << "Custom dimensions stay fixed when the terminal is resized.\n";
                return 0;
            }
            if (argument == "--size" || argument.starts_with("--size=")) {
                if (map_size) {
                    throw std::invalid_argument("Specify --size only once");
                }
                std::string_view dimensions;
                if (argument == "--size") {
                    if (++i == argc) {
                        throw std::invalid_argument("--size requires WIDTHxHEIGHT");
                    }
                    dimensions = argv[i];
                } else {
                    dimensions = argument.substr(7);
                }
                const auto separator = dimensions.find_first_of("xX");
                if (separator == std::string_view::npos) {
                    throw std::invalid_argument("--size requires WIDTHxHEIGHT");
                }
                const auto width = number(dimensions.substr(0, separator));
                const auto height = number(dimensions.substr(separator + 1));
                if (width == 0 || height == 0 || height > std::numeric_limits<std::size_t>::max() / width) {
                    throw std::invalid_argument("Map dimensions must be positive and their area must fit in size_t");
                }
                map_size = Size(width, height);
            } else {
                if (requested_seed || argument.starts_with('-')) {
                    throw std::invalid_argument("Usage: demo-features [SEED] [--size WIDTHxHEIGHT]");
                }
                const auto seed = number(argument);
                if (seed > std::numeric_limits<std::uint32_t>::max()) {
                    throw std::invalid_argument("Seed must fit in an unsigned 32-bit integer");
                }
                requested_seed = static_cast<std::uint32_t>(seed);
            }
        }
        const std::uint32_t seed = requested_seed ? *requested_seed : InitRandomness();
        InitRandomness(seed);
        CTileData::RegisterTile("pine", {U'♣', 0x2E7D32, false, false});
        CTileData::RegisterTile("pond", {U'≈', 0x3399FF, true, false});
        CTerminal terminal(true);
        CScreen screen;
        std::optional<CScreen> displayed;
        CWindowManager windows;
        windows.setAllowTabUndocking(false);
        windows.setAllowTabClosing(false);
        auto cave_glade_config = std::make_shared<CCaveGladeConfig>();
        std::vector<std::unique_ptr<CFeatureWindow>> tabs;
        const std::array titles = {"Map generators", "FOV", "Path finding", "Distance map", "Scheduler", "Calendar"};

        std::size_t active = 0;
        CCaveGladeConfigWindow config_window(cave_glade_config, [&] {
            if (!tabs.empty() && tabs[0]) {
                tabs[0]->Generate();
            }
        });
        bool configuring_cave = false;
        auto close_config = [&] {
            windows.Close(config_window);
            windows.Focus(*tabs[active]);
            configuring_cave = false;
        };
        auto open_config = [&] {
            if (active != 0) {
                active = 0;
                windows.Open(*tabs[0]);
            }
            tabs[0]->SelectGenerator("Cave and Glade");
            configuring_cave = true;
            for (auto& tab : tabs) tab->Hover(std::nullopt);
            const Size scr_size = screen.getSize();
            const std::size_t win_w = std::min<std::size_t>(50, scr_size.x > 4 ? scr_size.x - 2 : scr_size.x);
            const std::size_t win_h = std::min<std::size_t>(10, scr_size.y > 4 ? scr_size.y - 2 : scr_size.y);
            const Position pos(scr_size.x > win_w ? (scr_size.x - win_w) / 2 : 1,
                               scr_size.y > win_h ? (scr_size.y - win_h) / 2 : 1);
            windows.setLayout(config_window, SWindowLayout{
                .open = true,
                .dock = EDock::None,
                .floating = SRect(pos, Size(win_w, win_h))
            });
            windows.Open(config_window);
            windows.Focus(config_window);
        };

        for (std::size_t index = 0; index < titles.size(); ++index) {
            tabs.push_back(std::make_unique<CFeatureWindow>(titles[index], static_cast<int>(index), map_size,
                                                            cave_glade_config, [&] { open_config(); }));
            tabs.back()->setFrameTitle("RoguelikeLib features demo");
            windows.Open(*tabs.back());
            windows.Dock(*tabs.back(), EDock::Top, 1000);

            if (index > 0) {
                windows.Tab(*tabs.back(), *tabs.front());
            }
        }

        windows.Focus(*tabs.front());
        CMapSizeInput size_entry;
        bool editing_size = false;
        auto close_size = [&] {
            windows.Close(size_entry);
            windows.Focus(*tabs[active]);
            editing_size = false;
        };
        auto apply_size = [&](std::string_view value) {
            const auto separator = value.find_first_of("xX");
            std::size_t width = 0, height = 0;
            auto parse = [](std::string_view value, std::size_t& result) {
                const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
                return error == std::errc{} && end == value.data() + value.size();
            };
            if (separator == std::string_view::npos || !parse(value.substr(0, separator), width) ||
                    !parse(value.substr(separator + 1), height) || width == 0 || height == 0 ||
                    height > std::numeric_limits<std::size_t>::max() / width) {
                size_entry.setMessage("Use positive sizes, e.g. 200x100");
                return;
            }
            for (auto& tab : tabs) {
                tab->setMapSize(Size(width, height));
            }
            close_size();
        };
        CFeatureWindow* ticking = nullptr;
        auto next_tick = std::chrono::steady_clock::now();

        for (;;) {
            screen.Resize(terminal.getSize());
            screen.Clear();
            const Size size = screen.getSize();
            CFeatureWindow* visible_timer = nullptr;

            if (size.x >= 40 && size.y >= 10 && tabs[active]->IsRunning() && windows.getFocused() == tabs[active].get()) {
                visible_timer = tabs[active].get();
            }

            const auto now = std::chrono::steady_clock::now();

            if (visible_timer != ticking) {
                ticking = visible_timer;
                next_tick = now + std::chrono::seconds(1);
            } else if (ticking != nullptr && now >= next_tick) {
                ticking->Tick();
                next_tick = now + std::chrono::seconds(1);
            }


            if (size.x < 40 || size.y < 10) {
                screen.Print(0, 0, "Resize to at least 40x10; Q quits");
            } else {
                windows.Layout({Position(0, 0), Size(size.x, size.y - 1)});
                windows.Draw(screen);
                screen.Print(0, size.y - 1, "1-6/Tab: tabs | C: configure | M: map size | R: regenerate | hjkl: pan | Q: quit");
            }

            terminal.Write(screen.Compose(false, displayed ? &*displayed : nullptr));
            displayed = screen;
            const int timeout_ms = ticking == nullptr ? -1 : static_cast<int>(std::max<std::int64_t>(1,
                                                                                                     std::chrono::duration_cast<std::chrono::milliseconds>(next_tick - std::chrono::steady_clock::now()).count()));
            const SKey key = terminal.ReadKey(timeout_ms);

            if (key.type == EKey::Mouse) {
                for (auto& tab : tabs) {
                    tab->Hover((editing_size || configuring_cave) ? std::nullopt : std::optional(key.mouse.position));
                }
            }

            if (editing_size && key.type != EKey::Closed && !(key.type == EKey::Char && key.ch == 3)) {
                size_entry.HandleKey(key);
                if (size_entry.TakeCancelled()) {
                    close_size();
                } else if (const auto entered = size_entry.TakeEntered()) {
                    apply_size(*entered);
                }
                continue;
            }
            if (configuring_cave && key.type != EKey::Closed && !(key.type == EKey::Char && key.ch == 3)) {
                config_window.HandleKey(key);
                if (config_window.TakeDone()) {
                    close_config();
                }
                continue;
            }
            if (!editing_size && !configuring_cave && key.type == EKey::Char && (key.ch == U'c' || key.ch == U'C')) {
                open_config();
                continue;
            }
            if (!editing_size && !configuring_cave && key.type == EKey::Char && (key.ch == U'm' || key.ch == U'M')) {
                const auto dimensions = tabs[active < 4 ? active : 0]->getMapSize();
                size_entry.Begin(dimensions);
                editing_size = true;
                for (auto& tab : tabs) tab->Hover(std::nullopt);
                windows.Open(size_entry);
                windows.Focus(size_entry);
                continue;
            }
            if (key.type == EKey::Closed || key.type == EKey::Escape ||
                    (key.type == EKey::Char && (key.ch == U'q' || key.ch == U'Q' || key.ch == 3))) {
                break;
            }

            if (key.type == EKey::Mouse && size.x >= 40 && size.y >= 10) {
                windows.HandleMouse(key.mouse);

                if (configuring_cave && config_window.TakeDone()) {
                    close_config();
                }

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

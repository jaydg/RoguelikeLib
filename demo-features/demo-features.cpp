import rl.clock;
import rl.distancemap;
import rl.fov;
import rl.map;
import rl.mapgenerators;
import rl.names;
import rl.maputils;
import rl.pathfinding;
import rl.position;
import rl.randomness;
import rl.scheduler;
import rl.tile;
import rl.windows;
import std;
import stc;

using namespace std;

// A window that shows a few lines of text, to have something behind the
// tabs of windows docked to an edge
class CTextWindow : public RL::CWindow
{
private:
    std::vector<std::string> lines;

public:
    CTextWindow(std::string a_title, std::vector<std::string> a_lines)
        : RL::CWindow(std::move(a_title), RL::SRect{RL::Position(4, 4), RL::Size(30, 10)}),
          lines(std::move(a_lines))
    {
    }

    void Draw(RL::CScreen& screen, const RL::SRect& inside) override
    {
        for (std::size_t line = 0; line < lines.size(); ++line) {
            if (inside.corner.y + line >= inside.Bottom()) {
                break;
            }

            screen.Print(inside.corner.x, inside.corner.y + line, lines[line]);
        }
    }
};

int main(int argc, char* argv[])
{
    //////////////////////////////////////////////////////////////////////////
    // Initialization of randomness
    //////////////////////////////////////////////////////////////////////////

    // Pass a seed as first argument to generate the same levels again
    std::uint32_t seed;

    if (argc > 1) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[1]));
        RL::InitRandomness(seed);
    } else {
        seed = RL::InitRandomness();
    }

    cout << "Seed: " << seed << endl << endl;

    //////////////////////////////////////////////////////////////////////////
    // Define the map
    //////////////////////////////////////////////////////////////////////////

    const RL::Size level_size(79, 50);
    RL::CMap level = RL::CMap(level_size);

    //////////////////////////////////////////////////////////////////////////
    // Generate some levels
    //////////////////////////////////////////////////////////////////////////

    cout << stc::underline << "Standard Dungeon" << stc::reset << endl << endl;
    RL::CreateStandardDungeon(level, 20);
    level.PrintMap();

    cout << endl << stc::underline << "Ant's Nest" << stc::reset << endl << endl;
    RL::CreateAntNest(level, true);
    level.PrintMap();

    cout << endl << stc::underline << "Mines" << stc::reset << endl << endl;
    RL::CreateMines(level, 25);
    level.PrintMap();

    cout << endl << stc::underline << "Caves Sharp" << stc::reset << endl << endl;
    RL::CreateCaves(level, 1, 0.75);
    level.PrintMap();

    cout << endl << stc::underline << "Caves Soft" << stc::reset << endl << endl;
    RL::CreateCaves(level, 3);
    level.PrintMap();

    for (const auto& name : RL::GetDelvePresets()) {
        cout << endl << stc::underline << "Delve - " << name << stc::reset << endl << endl;

        RL::CreateDelve(level, name);
        level.PrintMap();
    }

    cout << endl << stc::underline << "Space shuttle" << stc::reset << endl << endl;
    RL::CreateSpaceShuttle(level, 25);
    level.PrintMap();

    cout << endl << stc::underline << "Castle" << stc::reset << endl << endl;
    RL::CreateSpaceShuttle(level, 25);
    level.PrintMap();

    cout << endl << stc::underline << "Forest" << stc::reset << endl << endl;
    RL::GenerateForest(level);
    level.PrintMap();

    cout << endl << stc::underline << "Cave and Glade - Cave" << stc::reset << endl << endl;
    RL::CreateCaveAndGlade(level, "wall", "room", RL::CaveShape, RL::CaveWater);
    level.PrintMap();

    cout << endl << stc::underline << "Cave and Glade - Small cave without water" << stc::reset << endl << endl;
    RL::CreateCaveAndGlade(level, "wall", "room", {.min_areas = 7, .max_areas = 11, .scale = 2}, {.shallow = {}});
    level.PrintMap();

    cout << endl << stc::underline << "Cave and Glade - Glade" << stc::reset << endl << endl;
    RL::CreateCaveAndGlade(level, "tree", "grass", RL::GladeShape, RL::GladeWater);
    level.PrintMap();

    // Tiles can be registered at runtime, with any Unicode symbol as glyph
    RL::CTileData::RegisterTile("pine", {U'♣', 0x2E7D32, false, false});
    RL::CTileData::RegisterTile("pond", {U'≈', 0x3399FF, true, false});

    cout << endl << stc::underline << "Cave and Glade - Glade with Unicode glyphs" << stc::reset << endl << endl;
    RL::CreateCaveAndGlade(level, "pine", "grass", RL::GladeShape, {.shallow = "pond", .level = 12, .deep_level = 35, .grain = 20});
    level.PrintMap();

    cout << endl << stc::underline << "Voronoi Wilderness" << stc::reset
         << " (difficulty 0 in the town, up to 9 in the hardest region)" << endl << endl;

    const RL::SWildernessInfo wilderness = RL::CreateVoronoiWilderness(level, 24, true);
    level.PrintMap();

    cout << wilderness.regions << " regions, " << wilderness.adjacency << " Delaunay edges";

    for (const RL::SLocation & location : wilderness.locations) {
        cout << ", town at (" << location.center.x << ", " << location.center.y
             << ") with radius " << location.radius;
    }

    for (const RL::SBuilding & building : wilderness.buildings) {
        cout << ", building from (" << building.corner1.x << ", " << building.corner1.y
             << ") to (" << building.corner2.x << ", " << building.corner2.y
             << ") with its door at (" << building.door.x << ", " << building.door.y << ")";
    }

    cout << endl;

    cout << endl << stc::underline << "Simple City with 15 buildings" << stc::reset << endl << endl;
    RL::CreateSimpleCity(level, 15);
    level.PrintMap();

    //////////////////////////////////////////////////////////////////////////
    // Field of view testing
    //////////////////////////////////////////////////////////////////////////

    cout << endl << stc::underline << "Field of View in Simple City from the road" << stc::reset << endl << endl;


    // Place observer somewhere on a horizontal road

    RL::Position observer;
    RL::FindOnMapRandomRectangleOfType(level, "corridor", observer, RL::Size(2, 1));

    // Define the FOV

    RL::CFOV fov(&level);

    // FOV prepared, calculate it

    fov.Calculate(observer, 9);

    // Print calculated FOV

    RL::Position pos;

    for (pos.y = 0; pos.y < level_size.y; ++pos.y) {
        for (pos.x = 0; pos.x < level_size.x; ++pos.x) {
            auto tile = level.get(pos);

            if (pos == observer) {
                cout << stc::rgb_fg(0xF0F000) << '@' << stc::reset;
            } else if (fov.get(pos)) { // visible cells take from the map
                cout << stc::rgb_fg(tile.getColor()) << RL::EncodeUTF8(tile.getGlyph()) << stc::reset;
            } else if (level.get(pos).getGlyph() == U'#') { // not visible
                cout << stc::rgb_fg(0x202020) << RL::EncodeUTF8(tile.getGlyph()) << stc::reset;
            } else { // others are empty
                cout << ' ';
            }
        }

        cout << endl;
    }

    //////////////////////////////////////////////////////////////////////////
    // Find path in the maze
    //////////////////////////////////////////////////////////////////////////

    // Create maze

    cout << endl << stc::underline << "Maze" << stc::reset << endl << endl;
    RL::CreateMaze(level);
    level.PrintMap();

    cout << endl << stc::underline << "Path in this maze" << stc::reset << " '+' (from top-left to bottom-right corner)" << endl << endl;

    // Find corners

    RL::Position start, end;

    for (pos.x = 0; pos.x < level_size.x; ++pos.x) {
        for (pos.y = 0; pos.y < level_size.y; ++pos.y) {
            if (level.get(pos).getType() == "corridor") {
                // set top-left corner
                if (start.x == RL::Position::invalid) {
                    start = pos;
                }

                // set bottom-right corner
                end = pos;
            }
        }
    }

    // find path in maze

    vector < RL::Position > path;
    RL::FindPath(level, start, end, path);

    // print maze with path

    for (std::size_t index = 0; index < path.size(); index++) {
        // this looks bogus, but we get a trail of '+' this way
        level.get(path[index].x, path[index].y).setType("door_closed");
    }

    level.PrintMap();

    //////////////////////////////////////////////////////////////////////////
    // Distance map: the way from everywhere to the nearest goal
    //////////////////////////////////////////////////////////////////////////

    cout << endl << stc::underline << "Distance map in a cave" << stc::reset
         << " (the last digit of the distance to '@', near in yellow, far in blue)" << endl << endl;

    RL::CreateCaves(level, 3);
    RL::Position goal;
    RL::FindOnMapRandomRectangleOfType(level, "room", goal, RL::Size(1, 1));
    const RL::CDistanceMap distances(level, std::array{goal});

    for (pos.y = 0; pos.y < level_size.y; ++pos.y) {
        for (pos.x = 0; pos.x < level_size.x; ++pos.x) {
            const int distance = distances.Distance(pos);

            if (pos == goal) {
                cout << stc::rgb_fg(0xF0F000) << '@';
            } else if (distance == RL::CDistanceMap::unreachable) {
                cout << stc::rgb_fg(0x404040) << (level.get(pos).isPassable() ? ' ' : '#');
            } else {
                // From yellow to blue over the first 60 steps
                const float far = static_cast<float>(std::min(distance, 60)) / 60.0f;
                cout << stc::hsl_fg(0.15f + 0.5f * far, 0.8f, 0.55f) << distance % 10;
            }
        }

        cout << stc::reset << endl;
    }

    //////////////////////////////////////////////////////////////////////////
    // Clock: the time of day, and what holds at it
    //////////////////////////////////////////////////////////////////////////

    cout << endl << stc::underline << "A day" << stc::reset << " (half an hour a turn)" << endl << endl;

    RL::CClock clock(30);
    RL::CDailySchedule<string_view> phases;
    phases.Set(6, 0, "morning");
    phases.Set(12, 0, "afternoon");
    phases.Set(18, 0, "evening");
    phases.Set(22, 0, "night");

    for (int turn = 0; turn < 48; turn += 6) {
        cout << clock.ToString() << ' ' << phases.At(clock) << endl;
        clock.Advance(6);
    }

    //////////////////////////////////////////////////////////////////////////
    // Scheduler: who acts when, by speed
    //////////////////////////////////////////////////////////////////////////

    cout << endl << stc::underline << "Scheduler" << stc::reset
         << " (a hare acts every 50 ticks, a fox every 100, a snail every 400)" << endl << endl;

    const map<string_view, RL::Ticks> delays = {{"hare", 50}, {"fox", 100}, {"snail", 400}};
    RL::CScheduler<string_view> scheduler;

    for (const auto& [name, delay] : delays) {
        scheduler.Schedule(name, 0);
    }

    while (auto actor = scheduler.Next()) {
        if (scheduler.Now() > 400) {
            break;
        }

        cout << scheduler.Now() << ": " << *actor << endl;
        scheduler.Schedule(*actor, delays.at(*actor));
    }

    //////////////////////////////////////////////////////////////////////////
    // Tabbed windows: one frame, one window of the group at a time
    //////////////////////////////////////////////////////////////////////////

    cout << endl << stc::underline << "Tabbed windows" << stc::reset
         << " (two windows docked left, sharing one frame under its tabs)" << endl << endl;

    // The composed screen prints row by row here: the jumps of the cursor
    // it asks the terminal for become line breaks
    const auto print_screen = [](const std::string & composed) {
        for (std::size_t pos = 0; pos < composed.size();) {
            if (composed[pos] != '\x1b' || pos + 1 >= composed.size() || composed[pos + 1] != '[') {
                cout << composed[pos++];
                continue;
            }

            std::size_t end = pos + 2;

            while (end < composed.size() && !(composed[end] >= 'A' && composed[end] <= 'Z') &&
                    !(composed[end] >= 'a' && composed[end] <= 'z')) {
                ++end;
            }

            if (end >= composed.size()) {
                cout << composed[pos++];
                continue;
            }

            if (composed[end] == 'H') {
                cout << endl;
            } else {
                cout << composed.substr(pos, end - pos + 1);
            }

            pos = end + 1;
        }
    };

    RL::CScreen tabs_screen(RL::Size(64, 14));
    RL::CWindowManager windows;

    CTextWindow stats("Stats", {"Health  <#FF4040>34/34", "Level   3", "Exp     120/200"});
    CTextWindow log("Log", {"You hit the rat.", "The rat flees.", "You pick up 12 gold."});

    windows.Open(stats);
    windows.Open(log);
    windows.Dock(stats, RL::EDock::Left, 22);
    windows.Dock(log, RL::EDock::Left, 22);
    windows.Tab(log, stats);

    windows.Layout({RL::Position(0, 0), tabs_screen.getSize()});
    windows.Draw(tabs_screen);
    print_screen(tabs_screen.Compose(false));

    // A click on the first tab of the strip shows its window in place of
    // the one that did; the strip is the row under the title, and the
    // first tab its box two columns in
    const RL::SRect tabbed = log.getFrame();

    RL::SMouse click;
    click.button = RL::EMouseButton::Left;
    click.position = RL::Position(tabbed.corner.x + 4, tabbed.corner.y + 1);
    windows.HandleMouse(click);

    cout << endl << "After a click on the first tab:" << endl << endl;

    tabs_screen.Clear();
    windows.Draw(tabs_screen);
    print_screen(tabs_screen.Compose(false));
    cout << endl;

    //////////////////////////////////////////////////////////////////////////
    // That's all folks!
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Names, made up from others
    //////////////////////////////////////////////////////////////////////////

    RL::CPersonNames names;

    for (const auto& [gender, title] : {
                std::pair(RL::EGender::Female, "Names of women"), std::pair(RL::EGender::Male, "Names of men")
            }) {
        cout << endl << stc::underline << title << stc::reset
             << " (of between 4 and 6 tokens, made from fifty common English ones)" << endl << endl;

        for (int count = 0; count < 15; ++count) {
            if (const auto name = names.Generate(gender)) {
                cout << *name << (count % 5 == 4 ? "\n" : "  ");
            }
        }
    }

    cout << endl;

    return 0;
}

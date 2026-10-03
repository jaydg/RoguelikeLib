import rl.fov;
import rl.map;
import rl.mapgenerators;
import rl.maputils;
import rl.pathfinding;
import rl.position;
import rl.randomness;
import std;
import stc;

using namespace std;

int main(void)
{
    //////////////////////////////////////////////////////////////////////////
    // Initialization of randomness
    //////////////////////////////////////////////////////////////////////////

    RL::InitRandomness();

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
                cout << stc::rgb_fg(tile.getColor()) << tile.getGlyph() << stc::reset;
            } else if (level.get(pos).getGlyph() == '#') { // not visible
                cout << stc::rgb_fg(0x202020) << tile.getGlyph() << stc::reset;
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
    // That's all folks!
    //////////////////////////////////////////////////////////////////////////
    return 0;
}

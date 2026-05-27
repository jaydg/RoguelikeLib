import rl.fov;
import rl.map;
import rl.mapgenerators;
import rl.maputils;
import rl.pathfinding;
import rl.position;
import rl.randomness;
import std;

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

    RL::CMap level;
    const RL::Size level_size(79, 50);
    level.Resize(level_size);

    //////////////////////////////////////////////////////////////////////////
    // Generate some levels
    //////////////////////////////////////////////////////////////////////////

    cout << endl << "Standard Dungeon" << endl << endl;
    RL::CreateStandardDungeon(level, 20);
    level.PrintMap();

    cout << endl << "Ant's Nest" << endl << endl;
    RL::CreateAntNest(level, true);
    level.PrintMap();

    cout << endl << "Mines" << endl << endl;
    RL::CreateMines(level, 25);
    level.PrintMap();

    cout << endl << "Caves Sharp" << endl << endl;
    RL::CreateCaves(level, 1, 0.75);
    level.PrintMap();

    cout << endl << "Caves Soft" << endl << endl;
    RL::CreateCaves(level, 3);
    level.PrintMap();

    for (const auto& name : RL::GetDelvePresets()) {
        cout << endl << "Delve - " << name << endl << endl;

        RL::CreateDelve(level, name);
        level.PrintMap();
    }

    cout << endl << "Space shuttle" << endl << endl;
    RL::CreateSpaceShuttle(level, 25);
    level.PrintMap();

    cout << endl << "Castle" << endl << endl;
    RL::CreateSpaceShuttle(level, 25);
    level.PrintMap();

    cout << endl << "Simple City with 15 buildings" << endl << endl;
    RL::CreateSimpleCity(level, 15);
    level.PrintMap();

    //////////////////////////////////////////////////////////////////////////
    // Field of view testing
    //////////////////////////////////////////////////////////////////////////

    cout << endl << "Field of View in Simple City from the road" << endl << endl;


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
            if (pos == observer) {
                cout << '@';
            } else if (fov(pos)) { // visible cells take from the map
                cout << (char)level.GetCell(pos).getGlyph();
            } else if (level.GetCell(pos).getGlyph() == '#') { // not visible walls as '%'
                cout << '%';
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

    cout << endl << "Maze" << endl << endl;
    RL::CreateMaze(level);
    level.PrintMap();

    cout << endl << "Path in this maze '+' (from top-left to bottom-right corner)" << endl << endl;

    // Find corners

    RL::Position start, end;

    for (pos.x = 0; pos.x < level_size.x; ++pos.x) {
        for (pos.y = 0; pos.y < level_size.y; ++pos.y) {
            if (level.GetCell(pos).getType() == "corridor") {
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
        level.GetCell(path[index].x, path[index].y).setType("door_closed");
    }

    level.PrintMap();

    //////////////////////////////////////////////////////////////////////////
    // That's all folks!
    //////////////////////////////////////////////////////////////////////////
    return 0;
}

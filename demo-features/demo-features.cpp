#include "../roguelikelib/randomness.h"
#include "../roguelikelib/mapgenerators.h"
#include "../roguelikelib/fov.h"
#include "../roguelikelib/pathfinding.h"

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

    cout << endl << "Space shuttle" << endl << endl;
    RL::CreateSpaceShuttle(level, 25);
    level.PrintMap();

    cout << endl << "Castle" << endl << endl;
    RL::CreateSpaceShuttle(level, 25, true);
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
    RL::FindOnMapRandomRectangleOfType(level, RL::LevelElementCorridor, observer, RL::Size(2, 1));

    // Define the FOV

    RL::CMap fov;
    fov.Resize(level_size);

    // Only walls block the FOV

    RL::Position pos;

    for(pos.x = 0; pos.x < level_size.x; ++pos.x) {
        for(pos.y = 0; pos.y < level_size.y; ++pos.y) {
            if(level.GetCell(pos) == RL::LevelElementWall || level.GetCell(pos) == RL::LevelElementDoorClose) {
                fov.SetCell(pos, 1); // blocks
            } else {
                fov.SetCell(pos, 0); // doesn't block
            }
        }
    }

    // FOV prepared, calculate it

    RL::CalculateFOV(fov, observer, 9);

    // Print calculated FOV

    for(pos.y = 0; pos.y < level_size.y; ++pos.y) {
        for(pos.x = 0; pos.x < level_size.x; ++pos.x) {
            if(pos == observer) {
                cout << '@';
            } else if(fov.GetCell(pos) == 1) { // visible cells take from the map
                cout << (char) level.GetCell(pos);
            } else if(level.GetCell(pos) == '#') { // not visible walls as '%'
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

    // convert tiles to values and find corners at the same time
    // conversion is needed for pathfinding because it uses flood fill algorithm
    // and you have to define what is blocking.

    RL::CMap temp_level = level; // copy the level to a temporary
    RL::Position start, end;

    for(pos.x = 0; pos.x < level_size.x; ++pos.x) {
        for(pos.y = 0; pos.y < level_size.y; ++pos.y) {
            if(temp_level.GetCell(pos) == RL::LevelElementCorridor) {
                temp_level.SetCell(pos, RL::LevelElementCorridor_value); // conversion

                // set top-left corner
                if(start.x == -1) {
                    start = pos;
                }

                // set bottom-right corner
                end = pos;
            } else {
                temp_level.SetCell(pos, RL::LevelElementWall_value); // conversion
            }
        }
    }

    // find path in maze

    vector < RL::Position > path;
    RL::FindPath(temp_level, start, end, path);

    // print maze with path

    for(size_t index = 0; index < path.size(); index++) {
        level.SetCell(path[index].x, path[index].y, '+');
    }

    level.PrintMap();

    //////////////////////////////////////////////////////////////////////////
    // That's all folks!
    //////////////////////////////////////////////////////////////////////////
    return 0;
}


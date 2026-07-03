import rl.fov;
import rl.map;
import rl.mapgenerators;
import rl.maputils;
import rl.pathfinding;
import rl.position;
import rl.randomness;
import rl.theta_star;
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
    // Theta* Pathfinding
    //////////////////////////////////////////////////////////////////////////

    cout << endl << stc::underline << "Theta* Pathfinding" << stc::reset << endl << endl;

    RL::CMap theta_map(RL::Size(40, 20));
    theta_map.Clear("room");

    // Add some obstacles
    for (int x = 10; x < 25; x++) {
        theta_map.SetCell(x, 8, "wall");
    }
    for (int y = 3; y < 15; y++) {
        theta_map.SetCell(20, y, "wall");
    }
    theta_map.SetCell(5, 5, "tree");
    theta_map.SetCell(35, 15, "tree");

    cout << "Map with obstacles:" << endl;
    theta_map.PrintMap();

    RL::Position theta_start(2, 2);
    RL::Position theta_end(38, 18);

    cout << endl << "Finding path from " << theta_start.toString()
              << " to " << theta_end.toString() << " using Theta*" << endl;

    RL::CThetaStar theta_star;
    std::vector<RL::Position> theta_path;
    bool theta_path_found = theta_star.FindPath(theta_map, theta_start, theta_end, theta_path, true);

    if (theta_path_found) {
        cout << "Theta* path found with " << theta_path.size() << " steps!" << endl;

        // Mark path on map
        for (const auto& pos : theta_path) {
            if (theta_map.inside(pos)) {
                theta_map.get(pos).setType("corridor");
            }
        }

        cout << endl << "Path (marked with '.')" << endl;
        theta_map.PrintMap();
    } else {
        cout << "No Theta* path found!" << endl;
    }

    // Test line of sight
    cout << endl << "Testing line of sight:" << endl;
    RL::Position los_a(3, 3);
    RL::Position los_b(35, 10);

    RL::SLineOfSightResult los_result = RL::CheckLineOfSight(theta_map, los_a, los_b);
    cout << "Line of sight from " << los_a.toString() << " to "
         << los_b.toString() << ": "
         << (los_result.visible ? "Visible" : "Blocked") << endl;

    if (!los_result.visible) {
        cout << "  Blocked at: " << los_result.blocking_pos.toString() << endl;
    }

    // Test Angle-Propagation Theta*
    cout << endl << "Testing Angle-Propagation Theta*:" << endl;
    std::vector<RL::Position> theta_ap_path;
    bool theta_ap_found = theta_star.FindPathAP(theta_map, theta_start, theta_end, theta_ap_path, true);

    if (theta_ap_found) {
        cout << "AP Theta* path found with " << theta_ap_path.size() << " steps!" << endl;
    } else {
        cout << "No AP Theta* path found!" << endl;
    }

    //////////////////////////////////////////////////////////////////////////
    // That's all folks!
    //////////////////////////////////////////////////////////////////////////
    return 0;
}

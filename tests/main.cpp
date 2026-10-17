// Tests for RoguelikeLib and RTUI, run with `xmake test`, each by itself;
// or some of them by name, with `xmake run roguelikelib-tests Screen Clock`

import rltests.check;
import std;

void TestUTF8();
void TestDecodeKey();
void TestMouse();
void TestScheduler();
void TestClock();
void TestCalendar();
void TestDistanceMap();
void TestNames();
void TestGrowthTown();
void TestVoronoiScale();
void TestVoronoiBogs();
void TestVoronoiChasms();
void TestVoronoiFields();
void TestWindingRoad();
void TestForestRoads();
void TestMapStamp();
void TestTownRoads();
void TestScreen();
void TestCamera();
void TestRectangles();
void TestWindowManager();
void TestDoubleClick();
void TestNarrowTabs();
void TestListMarkup();
void TestListDisabled();
void TestListFollowed();
void TestListShortcuts();
void TestInput();
void TestPathfinding();
void TestDistance();
void TestMatrix();
void TestRandomness();

int main(int argc, char* argv[])
{
    // Every test, by its name: all of them run, or only those named on the
    // command line. xmake.lua finds the names here, one test each.
    const std::vector<std::pair<std::string_view, void (*)()>> tests = {
        {"UTF8", TestUTF8},
        {"DecodeKey", TestDecodeKey},
        {"Mouse", TestMouse},
        {"Scheduler", TestScheduler},
        {"Clock", TestClock},
        {"Calendar", TestCalendar},
        {"DistanceMap", TestDistanceMap},
        {"Names", TestNames},
        {"GrowthTown", TestGrowthTown},
        {"VoronoiScale", TestVoronoiScale},
        {"VoronoiBogs", TestVoronoiBogs},
        {"VoronoiChasms", TestVoronoiChasms},
        {"VoronoiFields", TestVoronoiFields},
        {"WindingRoad", TestWindingRoad},
        {"ForestRoads", TestForestRoads},
        {"MapStamp", TestMapStamp},
        {"TownRoads", TestTownRoads},
        {"Screen", TestScreen},
        {"Camera", TestCamera},
        {"Rectangles", TestRectangles},
        {"WindowManager", TestWindowManager},
        {"DoubleClick", TestDoubleClick},
        {"NarrowTabs", TestNarrowTabs},
        {"ListMarkup", TestListMarkup},
        {"ListDisabled", TestListDisabled},
        {"ListFollowed", TestListFollowed},
        {"ListShortcuts", TestListShortcuts},
        {"Input", TestInput},
        {"Pathfinding", TestPathfinding},
        {"Distance", TestDistance},
        {"Matrix", TestMatrix},
        {"Randomness", TestRandomness}
    };
    const std::vector<std::string_view> named(argv + 1, argv + argc);

    for (const std::string_view name : named) {
        if (std::ranges::find(tests, name, &std::pair<std::string_view, void (*)()>::first) == tests.end()) {
            std::cerr << "there is no test " << name << '\n';
            return 1;
        }
    }

    for (const auto& [name, test] : tests) {
        if (named.empty() || std::ranges::find(named, name) != named.end()) {
            test();
        }
    }

    if (RL::tests::Failures() > 0) {
        std::cerr << RL::tests::Failures() << " of " << RL::tests::Checks() << " checks failed\n";
        return 1;
    }

    std::cout << "all " << RL::tests::Checks() << " checks passed\n";
    return 0;
}

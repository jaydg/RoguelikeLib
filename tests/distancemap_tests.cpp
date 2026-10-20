// Tests for RoguelikeLib's rl.distancemap

import rl.distancemap;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{

// Follows the map's steps from a position to a goal, adding up what each
// step costs; -1 if it does not get there
int Walk(const RL::CDistanceMap& distances, const RL::CMap& map, RL::Position from, std::span<const RL::SStepCost> costs)
{
    int total = 0;

    for (int steps = 0; steps < 1000; ++steps) {
        if (distances.Distance(from) == 0) {
            return total;
        }

        const auto next = distances.NextStep(from);

        if (!next) {
            return -1;
        }

        int cost = 1;

        for (const auto& listed : costs) {
            if (map.get(*next).getType() == listed.type) {
                cost = listed.cost;
            }
        }

        total += cost;
        from = *next;
    }

    return -1;
}

} // namespace

void TestDistanceMap()
{
    const RL::CMap open = RL::tests::MakeMap({
        ".....",
        ".....",
        ".....",
        ".....",
        ".....",
    });
    const std::array goal{RL::Position(0, 0)};

    const RL::CDistanceMap eight(open, goal);
    Check(eight.Distance(RL::Position(0, 0)) == 0 && eight.Distance(RL::Position(4, 4)) == 4 &&
          eight.Distance(RL::Position(4, 1)) == 4, "with diagonal steps");
    Check(!eight.NextStep(RL::Position(0, 0)).has_value(), "no step at the goal");
    Check(eight.NextStep(RL::Position(4, 4)) == RL::Position(3, 3), "the step leads towards the goal");

    const RL::CDistanceMap four(open, goal, {}, RL::Neighbors::Cardinal4);
    Check(four.Distance(RL::Position(4, 4)) == 8 && four.NextStep(RL::Position(4, 0)) == RL::Position(3, 0),
          "without diagonal steps");

    // Paths cheaper than grass keep walkers on the path
    const RL::CMap paths = RL::tests::MakeMap({
        "......",
        "::::::",
        "......",
    });
    const std::array costs{RL::SStepCost{"grass", 3}};
    const std::array corner{RL::Position(5, 0)};
    const RL::CDistanceMap cheap(paths, corner, costs);
    Check(cheap.Distance(RL::Position(0, 0)) == 7, "the cheapest way, along the path");
    Check(cheap.NextStep(RL::Position(0, 0)) == RL::Position(1, 1), "the first step goes onto the path");
    Check(Walk(cheap, paths, RL::Position(0, 0), costs) == 7, "following the steps costs what the distance says");
    Check(Walk(cheap, paths, RL::Position(0, 2), costs) == cheap.Distance(RL::Position(0, 2)), "from anywhere");

    // A cost names its type by its letters, wherever the name is kept; a
    // type nobody registered changes nothing, and the first cost given for
    // a type is the one that counts
    const std::string spelled = std::string("gra") + "ss";
    const std::array named{RL::SStepCost{"no such tile", 5}, RL::SStepCost{spelled, 3}, RL::SStepCost{"grass", 9}};
    const RL::CDistanceMap same(paths, corner, named);
    bool alike = true;

    for (std::size_t y = 0; y < 3; ++y) {
        for (std::size_t x = 0; x < 6; ++x) {
            alike = alike && same.Distance(RL::Position(x, y)) == cheap.Distance(RL::Position(x, y)) &&
                    same.NextStep(RL::Position(x, y)) == cheap.NextStep(RL::Position(x, y));
        }
    }

    Check(alike, "a name spelled at runtime, unknown types and later duplicates");

    // What cannot be walked on is not walked through
    const RL::CMap walled = RL::tests::MakeMap({
        "..T..",
        "..T..",
        "..T..",
    });
    const RL::CDistanceMap behind(walled, goal);
    Check(!behind.Reachable(RL::Position(4, 1)) && behind.Distance(RL::Position(4, 1)) == RL::CDistanceMap::unreachable &&
          !behind.NextStep(RL::Position(4, 1)).has_value(), "behind a wall there is no way");
    Check(!behind.Reachable(RL::Position(2, 1)), "nor on the wall");

    // A goal that cannot be stood on leads next to it
    const std::array tree{RL::Position(2, 1)};
    const RL::CDistanceMap well(walled, tree);
    Check(well.Distance(RL::Position(1, 1)) == 1 && well.NextStep(RL::Position(1, 1)) == RL::Position(2, 1),
          "the step from next to it leads onto it");
    Check(well.Distance(RL::Position(3, 1)) == 1 && well.Distance(RL::Position(4, 1)) == 2 &&
          well.Distance(RL::Position(0, 1)) == 2, "it can be reached from all sides");

    // Several goals: the nearest counts
    const std::array two{RL::Position(0, 0), RL::Position(4, 4)};
    const RL::CDistanceMap nearest(open, two);
    Check(nearest.Distance(RL::Position(3, 3)) == 1 && nearest.Distance(RL::Position(1, 1)) == 1 &&
          nearest.Distance(RL::Position(2, 2)) == 2, "the nearest goal counts");

    // Off the map, and costs too low
    Check(!eight.Reachable(RL::Position(9, 9)), "off the map there is no way");

    bool threw = false;

    try {
        const std::array free{RL::SStepCost{"grass", 0}};
        const RL::CDistanceMap wrong(open, goal, free);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "a step costing nothing is refused");

    // Measured only until some positions are known: those come out exactly
    // as the complete search has them, with or without differing costs
    const RL::CMap wide = RL::tests::MakeMap({
        "............",
        "::::::::::::",
        "....T.......",
        "....T.......",
        "............",
    });
    const std::array start{RL::Position(0, 0)};
    const std::array wanted{RL::Position(5, 2), RL::Position(6, 3), RL::Position(5, 4)};

    for (const bool costly : {true, false}) {
        const std::span<const RL::SStepCost> paid = costly ? std::span<const RL::SStepCost>(costs) : std::span<const RL::SStepCost> {};
        const RL::CDistanceMap complete(wide, start, paid);
        const RL::CDistanceMap partial(wide, start, paid, RL::Neighbors::All8, RL::CDistanceMap::unreachable, wanted);
        bool exact = true;

        for (const auto pos : wanted) {
            exact = exact && partial.Distance(pos) == complete.Distance(pos) && complete.Reachable(pos);
        }

        Check(exact, costly ? "the positions asked for are measured exactly, with costs" :
              "the positions asked for are measured exactly, at uniform cost");
    }

    // A position that cannot be reached lets the search run to its end
    const std::array beyond{RL::Position(4, 2), RL::Position(11, 4)};
    const RL::CDistanceMap complete(wide, start, costs);
    const RL::CDistanceMap partial(wide, start, costs, RL::Neighbors::All8, RL::CDistanceMap::unreachable, beyond);
    bool whole = true;

    for (std::size_t y = 0; y < wide.getHeight(); ++y) {
        for (std::size_t x = 0; x < wide.getWidth(); ++x) {
            whole = whole && partial.Distance(RL::Position(x, y)) == complete.Distance(RL::Position(x, y));
        }
    }

    Check(whole, "asked for one that cannot be reached, the whole map is measured");

    // Mended after tiles change, the distances and steps are exactly those
    // of the changed map measured anew: trees felled and planted, paths worn
    // in, water bridged, with and without costs, in eight and four
    // directions, over the whole map or only so far
    const std::array kinds{"grass", "corridor", "tree", "water", "bridge", "plant"};
    const std::array varied{RL::SStepCost{"grass", 3}, RL::SStepCost{"plant", 5}, RL::SStepCost{"water", 4}};
    std::mt19937 dice(4);
    bool as_new = true;
    int mended = 0;

    for (int round = 0; round < 40; ++round) {
        RL::CMap map = RL::tests::MakeMap({
            "..........T.......",
            "....TT....T..~~...",
            "::::::::::T..~~...",
            "..&&......=..~~...",
            "..&&...TTTT..~~...",
            "..........:.......",
            "TTTT......:...&&..",
            "..........:.......",
        });
        const std::array goals{RL::Position(17, 0), RL::Position(0, 7)};
        const std::span<const RL::SStepCost> paid = round % 2 == 0 ? std::span<const RL::SStepCost>(varied) :
                                                    std::span<const RL::SStepCost> {};
        const RL::Neighbors directions = round % 4 < 2 ? RL::Neighbors::All8 : RL::Neighbors::Cardinal4;
        const int reach = round % 3 == 0 ? 9 : RL::CDistanceMap::unreachable;
        RL::CDistanceMap repaired(map, goals, paid, directions, reach);

        for (int change = 0; change < 25; ++change) {
            std::vector<RL::Position> changed;

            for (std::uint32_t tiles = 1 + dice() % 4; tiles > 0; --tiles) {
                const RL::Position pos(dice() % map.getWidth(), dice() % map.getHeight());

                if (std::ranges::find(goals, pos) == goals.end()) {
                    map.SetCell(pos.x, pos.y, kinds[dice() % kinds.size()]);
                    changed.push_back(pos);
                }
            }

            as_new = as_new && repaired.Mend(map, changed);
            ++mended;
            const RL::CDistanceMap fresh(map, goals, paid, directions, reach);

            for (std::size_t y = 0; y < map.getHeight(); ++y) {
                for (std::size_t x = 0; x < map.getWidth(); ++x) {
                    const RL::Position pos(x, y);
                    as_new = as_new && repaired.Distance(pos) == fresh.Distance(pos) && repaired.NextStep(pos) == fresh.NextStep(pos);
                }
            }
        }
    }

    Check(as_new && mended == 1000, "mended after changes, the distances are those measured anew");

    // What cannot be mended is left as it was, to be measured anew
    RL::CMap changing = RL::tests::MakeMap({
        ".....",
        ".....",
    });
    const std::array corner_goal{RL::Position(0, 0)};
    const std::array far_corner{RL::Position(4, 1)};
    RL::CDistanceMap limited(changing, corner_goal, {}, RL::Neighbors::All8, 2);
    RL::CDistanceMap stopped(changing, corner_goal, {}, RL::Neighbors::All8, RL::CDistanceMap::unreachable, far_corner);
    RL::CDistanceMap whole_map(changing, corner_goal);
    changing.SetCell(2, 0, "tree");
    const std::array felled{RL::Position(2, 0)};
    const std::array off_map{RL::Position(9, 9)};
    const RL::CDistanceMap limited_anew(changing, corner_goal, {}, RL::Neighbors::All8, 2);
    bool limited_alike = limited.Mend(changing, felled);

    for (std::size_t x = 0; x < changing.getWidth(); ++x) {
        limited_alike = limited_alike && limited.Distance(RL::Position(x, 1)) == limited_anew.Distance(RL::Position(x, 1));
    }

    Check(limited_alike && !stopped.Mend(changing, felled), "a map measured only so far is mended, but not one measured until some are known");
    Check(!whole_map.Mend(changing, corner_goal) && !whole_map.Mend(changing, off_map) &&
          whole_map.Distance(RL::Position(4, 0)) == 4, "nor at a goal or off the map, and it stays as it was");

    // The nearest of some goals, searched for towards them, is the one a map
    // measured from the start finds nearest, the first of those as near
    std::mt19937 chance(7);
    bool nearest_alike = true;
    int searched = 0;
    int found = 0;

    for (int round = 0; round < 400; ++round) {
        RL::CMap map = RL::tests::MakeMap({
            "..........T.......",
            "....TT....T..~~...",
            "::::::::::T..~~...",
            "..&&......=..~~...",
            "..&&...TTTT..~~...",
            "..........:.......",
            "TTTT......:...&&..",
            "..........:.......",
        });

        for (int tiles = 0; tiles < 12; ++tiles) {
            map.SetCell(chance() % map.getWidth(), chance() % map.getHeight(), kinds[chance() % kinds.size()]);
        }

        const std::span<const RL::SStepCost> paid = round % 2 == 0 ? std::span<const RL::SStepCost>(varied) :
                                                    std::span<const RL::SStepCost> {};
        const RL::Neighbors directions = round % 4 < 2 ? RL::Neighbors::All8 : RL::Neighbors::Cardinal4;
        const RL::Position start(chance() % map.getWidth(), chance() % map.getHeight());
        std::vector<RL::Position> goals;

        for (std::uint32_t count = 1 + chance() % 8; count > 0; --count) {
            goals.emplace_back(chance() % (map.getWidth() + 2), chance() % map.getHeight());
        }

        if (round % 7 == 0) {
            goals.push_back(goals.front());
        }

        if (round % 11 == 0) {
            goals.push_back(start);
        }

        const RL::CDistanceMap measured(map, std::span(&start, 1), paid, directions);
        std::optional<std::pair<std::size_t, int>> expected;

        for (std::size_t index = 0; index < goals.size(); ++index) {
            const int distance = measured.Distance(goals[index]);

            if (distance != RL::CDistanceMap::unreachable && (!expected || distance < expected->second)) {
                expected = std::pair{index, distance};
            }
        }

        const auto nearest = RL::CDistanceMap::Nearest(map, start, goals, paid, directions);
        nearest_alike = nearest_alike && nearest == expected;
        ++searched;
        found += nearest ? 1 : 0;
    }

    Check(nearest_alike && searched == 400 && found > 200, "the nearest goal is the one a map from the start finds nearest");

    // Measured towards a position and the cells around it, the map has
    // their distances, and the step from it, exactly as the whole map does,
    // with costs and without, and within a distance limit
    bool towards_alike = true;
    int measured_towards = 0;

    for (int round = 0; round < 400; ++round) {
        RL::CMap map = RL::tests::MakeMap({
            "..........T.......",
            "....TT....T..~~...",
            "::::::::::T..~~...",
            "..&&......=..~~...",
            "..&&...TTTT..~~...",
            "..........:.......",
            "TTTT......:...&&..",
            "..........:.......",
        });

        for (int tiles = 0; tiles < 12; ++tiles) {
            map.SetCell(chance() % map.getWidth(), chance() % map.getHeight(), kinds[chance() % kinds.size()]);
        }

        const std::span<const RL::SStepCost> paid = round % 2 == 0 ? std::span<const RL::SStepCost>(varied) :
                                                    std::span<const RL::SStepCost> {};
        const RL::Neighbors directions = round % 4 < 2 ? RL::Neighbors::All8 : RL::Neighbors::Cardinal4;
        const int limit = round % 3 == 0 ? 12 : RL::CDistanceMap::unreachable;
        const RL::Position goal(chance() % map.getWidth(), chance() % map.getHeight());
        const RL::Position from(chance() % map.getWidth(), chance() % map.getHeight());
        std::vector<RL::Position> around;

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int x = static_cast<int>(from.x) + dx;
                const int y = static_cast<int>(from.y) + dy;

                if (x >= 0 && y >= 0) {
                    around.emplace_back(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
                }
            }
        }

        const RL::CDistanceMap whole(map, std::span(&goal, 1), paid, directions, limit);
        const RL::CDistanceMap towards(map, std::span(&goal, 1), paid, directions, limit, around);

        for (const auto pos : around) {
            towards_alike = towards_alike && towards.Distance(pos) == whole.Distance(pos);
        }

        towards_alike = towards_alike && towards.NextStep(from) == whole.NextStep(from);
        ++measured_towards;
    }

    Check(towards_alike && measured_towards == 400, "measured towards a position, its surroundings are as the whole map has them");
}

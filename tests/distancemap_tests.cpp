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
}

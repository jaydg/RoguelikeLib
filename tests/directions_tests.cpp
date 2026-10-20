// Tests for RoguelikeLib's rl.directions

import rl.directions;
import rl.position;
import rltests.check;
import std;

using RL::tests::Check;

void TestDirections()
{
    Check(RL::DirectionDelta(RL::North) == std::pair{0, -1} && RL::DirectionDelta(RL::SouthEast) == std::pair{1, 1} &&
          RL::DirectionDelta(RL::West) == std::pair{-1, 0}, "a step north goes up the map, south-east down and right");

    // Going round, each direction a step to one of the eight neighbours,
    // each once, and the opposite direction the opposite step
    std::set<std::pair<int, int>> steps;
    bool opposite = true;

    for (int direction = RL::EDirectionMin; direction <= RL::EDirectionMax; ++direction) {
        const auto [dx, dy] = RL::DirectionDelta(static_cast<RL::EDirection>(direction));
        const auto [ox, oy] = RL::DirectionDelta(static_cast<RL::EDirection>((direction + 4) % 8));
        steps.emplace(dx, dy);
        opposite = opposite && ox == -dx && oy == -dy;
    }

    Check(steps.size() == 8 && !steps.contains({0, 0}) && opposite, "the eight directions are the eight neighbours, opposites opposite");

    // The direction from a position to each of its neighbours, and none to
    // itself or farther
    const RL::Position from(1, 1);
    Check(from.DirectionTo(RL::Position(1, 0)) == RL::North && from.DirectionTo(RL::Position(2, 0)) == RL::NorthEast &&
          from.DirectionTo(RL::Position(2, 1)) == RL::East && from.DirectionTo(RL::Position(2, 2)) == RL::SouthEast &&
          from.DirectionTo(RL::Position(1, 2)) == RL::South && from.DirectionTo(RL::Position(0, 2)) == RL::SouthWest &&
          from.DirectionTo(RL::Position(0, 1)) == RL::West && from.DirectionTo(RL::Position(0, 0)) == RL::NorthWest,
          "the direction to each neighbour");
    Check(!from.DirectionTo(from) && !from.DirectionTo(RL::Position(3, 3)), "none to the same position, nor to one farther");
}

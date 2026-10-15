// Tests for RoguelikeLib's rl.distance

import rl.distance;
import rl.position;
import rltests.check;
import std;

using RL::tests::Check;

void TestDistance()
{
    // 1. Same point distance is zero
    Check(RL::Distance(0, 0, 0, 0) == 0, "distance to same point at origin is 0");
    Check(RL::Distance(42, 100, 42, 100) == 0, "distance to same point elsewhere is 0");

    // 2. Cardinal / orthogonal distance
    Check(RL::Distance(0, 0, 5, 0) == 5, "horizontal distance from origin");
    Check(RL::Distance(10, 3, 2, 3) == 8, "horizontal distance reversed");
    Check(RL::Distance(0, 0, 0, 7) == 7, "vertical distance from origin");
    Check(RL::Distance(4, 12, 4, 3) == 9, "vertical distance reversed");

    // 3. Exact Pythagorean triples
    Check(RL::Distance(0, 0, 3, 4) == 5, "3-4-5 right triangle from origin");
    Check(RL::Distance(10, 20, 13, 24) == 5, "3-4-5 right triangle offset");
    Check(RL::Distance(0, 0, 6, 8) == 10, "6-8-10 right triangle");
    Check(RL::Distance(0, 0, 5, 12) == 13, "5-12-13 right triangle");
    Check(RL::Distance(0, 0, 8, 15) == 17, "8-15-17 right triangle");

    // 4. Ceiling rounding behavior
    // sqrt(1^2 + 1^2) = sqrt(2) ≈ 1.414 -> ceil is 2
    Check(RL::Distance(0, 0, 1, 1) == 2, "diagonal unit step rounds up to 2");
    // sqrt(1^2 + 2^2) = sqrt(5) ≈ 2.236 -> ceil is 3
    Check(RL::Distance(0, 0, 1, 2) == 3, "step (1,2) rounds up to 3");
    // sqrt(2^2 + 3^2) = sqrt(13) ≈ 3.606 -> ceil is 4
    Check(RL::Distance(0, 0, 2, 3) == 4, "step (2,3) rounds up to 4");

    // 5. Symmetry
    Check(RL::Distance(3, 7, 9, 15) == RL::Distance(9, 15, 3, 7), "distance is symmetric");

    // 6. Large coordinates (no memory allocation or overflow)
    // sqrt(1000^2 + 1000^2) = sqrt(2,000,000) ≈ 1414.21 -> ceil is 1415
    Check(RL::Distance(0, 0, 1000, 1000) == 1415, "large coordinates calculate accurately");

    // 7. Position integration
    const RL::Position p1(10, 10);
    const RL::Position p2(13, 14);
    Check(p1.Distance(p2) == 5, "Position::Distance produces 5 for 3-4 delta");
    Check(p2.Distance(p1) == 5, "Position::Distance is symmetric");

    // 8. Noexcept verification
    static_assert(noexcept(RL::Distance(0, 0, 0, 0)));
}

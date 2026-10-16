// Tests for RoguelikeLib's rl.matrix

import rl.matrix;
import rl.position;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{
struct Tracker {
    int value{0};
    static inline int copy_count = 0;
    static inline int move_count = 0;

    Tracker() = default;
    Tracker(int v) : value(v) {}
    Tracker(const Tracker& other) : value(other.value) { ++copy_count; }
    Tracker(Tracker&& other) noexcept : value(other.value) { ++move_count; }
    Tracker& operator=(const Tracker& other) { value = other.value; ++copy_count; return *this; }
    Tracker& operator=(Tracker&& other) noexcept { value = other.value; ++move_count; return *this; }
};
}

void TestMatrix()
{
    // 1. set and set_unchecked with const-ref and rvalue-ref
    {
        RL::CMatrix<Tracker> mat(RL::Size(3, 3), Tracker(0));
        Tracker::copy_count = 0;
        Tracker::move_count = 0;

        // Lvalue assignment: must do exactly 1 copy (into vector), not 2
        Tracker t1(42);
        mat.set(1, 1, t1);
        Check(Tracker::copy_count == 1, "set with lvalue performs exactly one copy");
        Check(Tracker::move_count == 0, "set with lvalue performs zero moves");
        Check(mat.get(1, 1).value == 42, "value correctly stored via set lvalue");

        Tracker::copy_count = 0;
        Tracker::move_count = 0;

        // Rvalue assignment: must do 0 copies, only move
        mat.set(1, 1, Tracker(99));
        Check(Tracker::copy_count == 0, "set with rvalue performs zero copies");
        Check(Tracker::move_count > 0, "set with rvalue moves into matrix");
        Check(mat.get(1, 1).value == 99, "value correctly stored via set rvalue");

        Tracker::copy_count = 0;
        Tracker::move_count = 0;

        // set_unchecked with rvalue
        mat.set_unchecked(0, 0, Tracker(77));
        Check(Tracker::copy_count == 0, "set_unchecked with rvalue performs zero copies");
        Check(mat.get_unchecked(0, 0).value == 77, "value correctly stored via set_unchecked");

        // set with Position overload
        Tracker::copy_count = 0;
        mat.set(RL::Position(2, 2), t1);
        Check(Tracker::copy_count == 1, "set(Position, lvalue) performs exactly one copy");
        Check(mat.get(RL::Position(2, 2)).value == 42, "value stored via Position overload");
    }

    // 2. inside bounds checks and noexcept guarantees
    {
        const RL::CMatrix<int> mat(RL::Size(5, 10), 0);
        static_assert(noexcept(mat.inside(0, 0)));
        static_assert(noexcept(mat.inside(RL::Position(0, 0))));

        Check(mat.inside(0, 0), "top-left (0,0) is inside");
        Check(mat.inside(4, 9), "bottom-right (4,9) is inside");
        Check(!mat.inside(5, 9), "(5,9) is outside horizontally");
        Check(!mat.inside(4, 10), "(4,10) is outside vertically");
        Check(!mat.inside(100, 100), "far point is outside");
        Check(mat.inside(RL::Position(2, 5)), "Position(2,5) is inside");
        Check(!mat.inside(RL::Position(5, 5)), "Position(5,5) is outside");
    }

    // 3. Out-of-bounds exception throwing
    {
        RL::CMatrix<int> mat(RL::Size(3, 4), 10);
        bool caught_get = false;
        try {
            [[maybe_unused]] auto val = mat.get(3, 4);
        } catch (const RL::EOutOfBoundException& e) {
            caught_get = true;
            Check(std::string_view(e.what()).find("out of bounds") != std::string_view::npos,
                  "exception contains 'out of bounds'");
        }
        Check(caught_get, "get(3, 4) throws EOutOfBoundException");

        bool caught_set = false;
        try {
            mat.set(RL::Position(5, 5), 99);
        } catch (const RL::EOutOfBoundException&) {
            caught_set = true;
        }
        Check(caught_set, "set(Position(5, 5)) throws EOutOfBoundException");
    }
}

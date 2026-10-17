// Tests for rl.randomness and xoshiro256 PRNG engine

import rl.randomness;
import xoshiro256;
import rltests.check;
import std;

using RL::tests::Check;

void TestRandomness()
{
    // UniformRandomBitGenerator concept verification
    static_assert(std::uniform_random_bit_generator<RL::Xoshiro256PlusPlus>);
    static_assert(std::uniform_random_bit_generator<RL::Xoshiro256StarStar>);
    static_assert(std::uniform_random_bit_generator<RL::SplitMix64>);

    // Basic properties of Xoshiro256PlusPlus
    RL::Xoshiro256PlusPlus rng1(12345);
    RL::Xoshiro256PlusPlus rng2(12345);
    Check(rng1 == rng2, "generators initialized with same seed have identical state");

    const auto val1 = rng1();
    const auto val2 = rng2();
    Check(val1 == val2, "generators with same seed produce identical first value");
    Check(rng1 == rng2, "state advances identically");

    // Generators with different seeds produce different outputs
    RL::Xoshiro256PlusPlus rng_diff(54321);
    Check(rng1() != rng_diff(), "different seeds produce different sequences");

    // Zero-seed edge case handled by SplitMix64
    RL::Xoshiro256PlusPlus rng_zero(0);
    const auto zero_s = rng_zero.getState();
    Check((zero_s[0] | zero_s[1] | zero_s[2] | zero_s[3]) != 0,
          "seeding with 0 does not produce all-zero state");

    // Serialization via operator<< and operator>>
    RL::Xoshiro256PlusPlus rng_saved(999);
    for (int i = 0; i < 50; ++i) {
        rng_saved();
    }
    std::ostringstream oss;
    oss << rng_saved;

    RL::Xoshiro256PlusPlus rng_restored;
    std::istringstream iss(oss.str());
    iss >> rng_restored;
    Check(!iss.fail(), "stream deserialization succeeds");
    Check(rng_saved == rng_restored, "deserialized generator matches saved generator");

    // Further outputs must match exactly
    for (int i = 0; i < 20; ++i) {
        Check(rng_saved() == rng_restored(), "restored generator reproduces exact sequence");
    }

    // Invalid deserialization handling
    RL::Xoshiro256PlusPlus rng_invalid;
    std::istringstream bad_iss("0 0 0 0");
    bad_iss >> rng_invalid;
    Check(bad_iss.fail(), "deserializing all-zero state sets failbit");

    std::istringstream text_iss("invalid text state");
    bad_iss >> rng_invalid;
    Check(bad_iss.fail(), "deserializing non-numeric text sets failbit");

    // Jump function verification
    RL::Xoshiro256PlusPlus jump_rng(42);
    RL::Xoshiro256PlusPlus jump_copy = jump_rng;
    jump_rng.jump();
    Check(jump_rng != jump_copy, "jump() advances generator state");

    // Xoshiro256StarStar verification
    RL::Xoshiro256StarStar star1(777);
    RL::Xoshiro256StarStar star2(777);
    Check(star1() == star2(), "xoshiro256** produces deterministic sequences");

    // rl.randomness Module API Tests
    RL::InitRandomness(12345);
    const unsigned r1 = RL::Random(100);
    const unsigned r2 = RL::Random(100);
    const float f1 = RL::RandomFloat(0.0f, 10.0f);

    RL::InitRandomness(12345);
    Check(RL::Random(100) == r1, "InitRandomness reproduces first random number");
    Check(RL::Random(100) == r2, "InitRandomness reproduces second random number");
    Check(RL::RandomFloat(0.0f, 10.0f) == f1, "InitRandomness reproduces float random number");

    // Random(0) boundary
    Check(RL::Random(0) == 0, "Random(0) returns 0");

    // Random(limit) bounds
    for (int i = 0; i < 100; ++i) {
        const unsigned r = RL::Random(10);
        Check(r < 10, "Random(10) is always < 10");
    }

    // RandomBetween bounds
    Check(RL::RandomBetween(5, 5) == 5, "RandomBetween(x, x) returns x");
    for (int i = 0; i < 100; ++i) {
        const unsigned r = RL::RandomBetween(10, 20);
        Check(r >= 10 && r < 20, "RandomBetween(10, 20) is within [10, 20)");
    }

    // RandomFloat bounds
    for (int i = 0; i < 100; ++i) {
        const float f = RL::RandomFloat(2.5f, 5.5f);
        Check(f >= 2.5f && f < 5.5f, "RandomFloat is within [2.5, 5.5)");
    }

    // CoinToss
    bool saw_heads = false;
    bool saw_tails = false;
    for (int i = 0; i < 100; ++i) {
        if (RL::CoinToss()) saw_heads = true;
        else saw_tails = true;
    }
    Check(saw_heads && saw_tails, "CoinToss produces both heads and tails");

    // RandomLowerThatLimit
    Check(!RL::RandomLowerThatLimit(5, 0), "RandomLowerThatLimit returns false for value 0");
    Check(RL::RandomLowerThatLimit(100, 10), "RandomLowerThatLimit returns true when limit > value");

    // Shuffle
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const auto original = numbers;
    RL::Shuffle(numbers.begin(), numbers.end());
    Check(numbers.size() == original.size(), "Shuffle preserves element count");
    auto sorted = numbers;
    std::ranges::sort(sorted);
    Check(sorted == original, "Shuffle retains all elements");

    // GetRandomState / SetRandomState
    RL::InitRandomness(42);
    const std::string saved_state = RL::GetRandomState();
    std::vector<unsigned> seq1;
    for (int i = 0; i < 20; ++i) {
        seq1.push_back(RL::Random(1000));
    }

    // Restore state
    Check(RL::SetRandomState(saved_state), "SetRandomState succeeds for valid state");
    std::vector<unsigned> seq2;
    for (int i = 0; i < 20; ++i) {
        seq2.push_back(RL::Random(1000));
    }
    Check(seq1 == seq2, "restoring random state reproduces identical future sequence");

    // Invalid state handling
    Check(!RL::SetRandomState("garbage random state"), "SetRandomState rejects invalid string");
    Check(!RL::SetRandomState("0 0 0 0"), "SetRandomState rejects all-zero state");

    // Jittered color
    const std::uint32_t gray = 0x808080;
    const std::uint32_t jittered_gray = RL::GetJitteredColor(gray);
    const auto r = (jittered_gray >> 16) & 0xFF;
    const auto g = (jittered_gray >> 8) & 0xFF;
    const auto b = jittered_gray & 0xFF;
    Check(r == g && g == b, "grayscale jittering preserves monochromatic channels");

    const std::uint32_t color = 0x1080FF;
    const std::uint32_t jittered_color = RL::GetJitteredColor(color);
    Check(jittered_color != 0, "color jittering produces non-zero color");
}

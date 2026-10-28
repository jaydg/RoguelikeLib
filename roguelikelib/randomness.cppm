/**
 * @file randomness.cppm
 * @brief High-performance pseudo-random number generator routines and utilities.
 */

module;

//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

export module rl.randomness;

export import xoshiro256;
import std;

namespace RL
{

// Internal module state (not exported, has module linkage)
Xoshiro256PlusPlus rng;

// The engine's draws are mapped onto a range here, not by the standard library's
// distributions: the standard prescribes what an engine yields, but not how a
// distribution uses it, so libc++ and libstdc++ make different numbers of one seed.
// These follow libstdc++'s algorithms for an engine of full 64 bits, so that a seed
// gives what it gave before, with either library.

// The 128-bit product of two 64-bit numbers, as its high and its low half
std::pair<std::uint64_t, std::uint64_t> Product(const std::uint64_t a, const std::uint64_t b)
{
    const std::uint64_t a_low = a & 0xffffffffU;
    const std::uint64_t a_high = a >> 32;
    const std::uint64_t b_low = b & 0xffffffffU;
    const std::uint64_t b_high = b >> 32;
    const std::uint64_t low_low = a_low * b_low;
    const std::uint64_t high_low = a_high * b_low;
    const std::uint64_t low_high = a_low * b_high;
    const std::uint64_t cross = (low_low >> 32) + (high_low & 0xffffffffU) + low_high;
    return {a_high * b_high + (high_low >> 32) + (cross >> 32), (cross << 32) | (low_low & 0xffffffffU)};
}

// A number in [0, count), by Lemire's nearly divisionless method (as libstdc++'s
// uniform_int_distribution); a count of 0 stands for all 2^64 numbers
std::uint64_t Below(const std::uint64_t count)
{
    if (count == 0) {
        return rng();
    }

    auto [high, low] = Product(rng(), count);

    if (low < count) {
        const std::uint64_t threshold = (0 - count) % count;

        while (low < threshold) {
            std::tie(high, low) = Product(rng(), count);
        }
    }

    return high;
}

// A number in [0, 1), of the low 32 bits of a draw (as libstdc++'s
// generate_canonical<float>), drawn again should it round up to 1
float Canonical()
{
    while (true) {
        const float value = static_cast<float>(static_cast<std::uint32_t>(rng())) / 4294967296.0f;

        if (value < 1.0f) {
            return value;
        }
    }
}

// Public API (exported)
export {

    /**
     * @brief Seeds the pseudo-random number generator with a deterministic 64-bit value.
     *
     * Initializes the internal 256-bit xoshiro256++ engine state via SplitMix64.
     * Use this function to ensure identical sequences of random numbers for reproducible runs,
     * procedural map generation, or unit testing.
     *
     * @param seed 64-bit integer seed value.
     */
    void InitRandomness(const std::uint64_t seed)
    {
        rng.seed(seed);
    }

    /**
     * @brief Seeds the generator from a non-deterministic hardware entropy source (`std::random_device`).
     *
     * @return The 32-bit seed value drawn from entropy, allowing it to be logged
     *         and passed to @ref InitRandomness(const std::uint64_t) to reproduce the run.
     */
    std::uint32_t InitRandomness()
    {
        std::random_device rd;
        const std::uint32_t seed = rd();

        InitRandomness(seed);
        return seed;
    }

    /**
     * @brief Serializes the current PRNG state into a text string.
     *
     * Enables saving and restoring game state without altering the future sequence
     * of random numbers.
     *
     * @return Formatted string representation of the internal state.
     */
    std::string GetRandomState()
    {
        std::ostringstream state;
        state << rng;
        return state.str();
    }

    /**
     * @brief Restores the PRNG state from a serialized text string.
     *
     * If the string contains valid state text previously produced by @ref GetRandomState,
     * the generator's state is replaced. Otherwise, the generator remains unchanged.
     *
     * @param text Serialized state string.
     * @return True if the state was valid and successfully restored; false otherwise.
     */
    bool SetRandomState(std::string_view text)
    {
        std::istringstream state{std::string(text)};
        Xoshiro256PlusPlus restored;
        state >> restored;

        if (state.fail() || !(state >> std::ws).eof()) {
            return false;
        }

        rng = restored;
        return true;
    }

    /**
     * @brief Generates a uniformly distributed pseudo-random integer in the half-open range `[0, limit)`.
     *
     * @param limit Upper exclusive bound. If 0, returns 0.
     * @return Random integer in `[0, limit - 1]`.
     */
    unsigned Random(const std::size_t limit)
    {
        if (limit == 0) {
            return 0;
        }

        return static_cast<unsigned>(Below(limit));
    }

    /**
     * @brief Generates a uniformly distributed pseudo-random integer in the half-open range `[min, max)`.
     *
     * @param min Lower inclusive bound.
     * @param max Upper exclusive bound.
     * @return Random integer in `[min, max - 1]`. If `min == max`, returns `min`.
     */
    unsigned RandomBetween(const std::size_t min, const std::size_t max)
    {
        if (min == max) {
            return min;
        }

        return static_cast<unsigned>(min + Below(max - min));
    }

    /**
     * @brief Generates a uniformly distributed floating-point number in the half-open range `[min, max)`.
     *
     * @param min Lower inclusive bound (defaults to 0.0f).
     * @param max Upper exclusive bound (defaults to 1.0f).
     * @return Random float in `[min, max)`.
     */
    float RandomFloat(float min = 0.0, float max = 1.0)
    {
        return Canonical() * (max - min) + min;
    }

    /**
     * @brief Tests if a random integer in `[0, value)` is strictly less than `limit`.
     *
     * Evaluates a probability of @f$ \min(1.0, \frac{\text{limit}}{\text{value}}) @f$.
     *
     * @param limit Threshold value.
     * @param value Range denominator (`[0, value)`). Returns false if `value == 0`.
     * @return True if `Random(value) < limit`.
     */
    bool RandomLowerThatLimit(const std::size_t limit, const std::size_t value)
    {
        if (value == 0) {
            return false;
        }

        return Random(value) < limit;
    }

    /**
     * @brief Simulates a fair 50/50 coin toss.
     *
     * @return True or false with equal 50% probability.
     */
    bool CoinToss()
    {
        return Random(2) != 0;
    }

    /**
     * @brief Randomly shuffles the elements in the range `[first, last)` using the Fisher-Yates algorithm.
     *
     * Operates in @f$ O(n) @f$ time and @f$ O(1) @f$ auxiliary space.
     *
     * @tparam RandomAccessIterator Iterator type satisfying RandomAccessIterator.
     * @param first Beginning of the range to shuffle.
     * @param last End of the range to shuffle.
     */
    template <class RandomAccessIterator>
    void Shuffle(RandomAccessIterator first, RandomAccessIterator last)
    {
        for (auto i = (last - first) - 1; i > 0; --i) {
            std::swap(first[i], first[static_cast<decltype(i)>(Below(static_cast<std::uint64_t>(i) + 1))]);
        }
    }

    /**
     * @brief Slightly perturbs each color channel of an RGB integer by approximately ±10%.
     *
     * Designed to add subtle visual variation to terrain and walls without jarring shifts:
     * - Grayscale colors (`R == G == B`): Adjusts luminosity uniformly so the color remains neutral gray.
     * - Chromatic colors: Perturbs each channel independently within `[0.9, 1.11]`.
     *
     * @param base_color Color packed as `0x00RRGGBB`.
     * @return Jittered color packed as `0x00RRGGBB`.
     */
    std::uint32_t GetJitteredColor(std::uint32_t base_color)
    {
        // Extract color channels
        std::uint8_t r = (base_color >> 16) & 0xFF;
        std::uint8_t g = (base_color >> 8) & 0xFF;
        std::uint8_t b = base_color & 0xFF;

        // Check if this is a grayscale color (all channels equal)
        bool is_grayscale = (r == g && g == b);

        std::uint32_t new_r, new_g, new_b;

        if (is_grayscale) {
            // For grayscale: apply same factor to all channels (modify luminosity)
            float factor = RandomFloat(0.9f, 1.11f);
            new_r = static_cast<std::uint32_t>(std::clamp(r * factor, 0.0f, 255.0f));
            new_g = new_r;
            new_b = new_r;
        } else {
            // For colored: scramble each channel independently
            new_r = static_cast<std::uint32_t>(std::clamp(r * RandomFloat(0.9f, 1.11f), 0.0f, 255.0f));
            new_g = static_cast<std::uint32_t>(std::clamp(g * RandomFloat(0.9f, 1.11f), 0.0f, 255.0f));
            new_b = static_cast<std::uint32_t>(std::clamp(b * RandomFloat(0.9f, 1.11f), 0.0f, 255.0f));
        }

        // Pack color channels
        return (new_r << 16) | (new_g << 8) | new_b;
    }

} // end of export

} // end of namespace RL

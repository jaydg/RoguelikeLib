//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

module;

export module rl.randomness;

import std;

namespace RL
{

// Internal module state (not exported, has module linkage)
std::mt19937 mt;

// Public API (exported)
export {

    // Seed with a fixed value, so that the same sequence of calls
    // generates the same maps again
    void InitRandomness(const std::uint32_t seed)
    {
        mt.seed(seed);
    }

    // Seed from a non-deterministic source. The seed is returned, so that
    // it can be logged and passed to InitRandomness(seed) to replay a run.
    std::uint32_t InitRandomness()
    {
        std::random_device rd;
        const std::uint32_t seed = rd();

        InitRandomness(seed);
        return seed;
    }

    unsigned Random(const std::size_t limit)
    {
        if (limit == 0) {
            return 0;
        }

        std::uniform_int_distribution<std::size_t> dist(0, limit - 1);
        return static_cast<unsigned>(dist(mt));
    }

    // Generate a random number between min and max
    unsigned RandomBetween(const std::size_t min, const std::size_t max)
    {
        if (min == max) {
            return min;
        }

        std::uniform_int_distribution<std::size_t> dist(min, max - 1);
        return static_cast<unsigned>(dist(mt));
    }

    // Generate a random float in the range [min, max)
    // Default range is [0.0, 1.0)
    float RandomFloat(float min = 0.0, float max = 1.0)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(mt);
    }

    bool RandomLowerThatLimit(const std::size_t limit, const std::size_t value)
    {
        if (value == 0) {
            return false;
        }

        return Random(value) < limit;
    }

    bool CoinToss()
    {
        return Random(2) != 0;
    }

    template <class RandomAccessIterator>
    void Shuffle(RandomAccessIterator first, RandomAccessIterator last)
    {
        for (auto i = (last - first) - 1; i > 0; --i) {
            std::uniform_int_distribution<decltype(i)> d(0, i);
            std::swap(first[i], first[d(mt)]);
        }
    }

    // Jitter each color channel by 10%
    // For grayscale colors (R == G == B), modify only luminosity by +/- 10%
    // For colored colors, scramble each channel independently by +/- 10%
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

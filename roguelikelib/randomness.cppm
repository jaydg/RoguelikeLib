//////////////////////////////////////////////////////////////////////////
// Randomness
//////////////////////////////////////////////////////////////////////////

module;

export module rl.randomness;

import std;

namespace RL {

// Internal module state (not exported, has module linkage)
std::mt19937 mt;

// Public API (exported)
export {

    void InitRandomness()
    {
        std::random_device rd; // non-deterministic generator
        mt.seed(rd());         // to seed mersenne twister.
    }

    unsigned Random(const std::size_t limit)
    {
        if(limit == 0) {
            return 0;
        }

        std::uniform_int_distribution<std::size_t> dist(0, limit - 1);
        return static_cast<unsigned>(dist(mt));
    }

    bool RandomLowerThatLimit(const std::size_t limit, const std::size_t value)
    {
        if(value == 0) {
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
        for(auto i = (last - first) - 1; i > 0; --i) {
            std::uniform_int_distribution<decltype(i)> d(0, i);
            std::swap(first[i], first[d(mt)]);
        }
    }

} // end of export

} // end of namespace RL

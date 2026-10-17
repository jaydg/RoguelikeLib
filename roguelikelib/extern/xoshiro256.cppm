// Written in 2018/2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
//
// To the extent possible under law, the author has dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

module;

#include <cstdint>

export module xoshiro256;

import std;

export namespace RL
{

// Bitwise rotation helper
[[nodiscard]] constexpr std::uint64_t rotl(std::uint64_t x, int k) noexcept
{
    return (x << k) | (x >> (64 - k));
}

// SplitMix64 generator, recommended by Blackman and Vigna to initialize
// 256-bit state from a 64-bit seed.
class SplitMix64
{
private:
    std::uint64_t state = 0;

public:
    using result_type = std::uint64_t;

    constexpr explicit SplitMix64(std::uint64_t seed = 0) noexcept
        : state(seed)
    {
    }

    static constexpr result_type min() noexcept
    {
        return 0;
    }

    static constexpr result_type max() noexcept
    {
        return std::numeric_limits<result_type>::max();
    }

    constexpr void seed(std::uint64_t s) noexcept
    {
        state = s;
    }

    constexpr result_type operator()() noexcept
    {
        return next();
    }

    constexpr std::uint64_t next() noexcept
    {
        std::uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};

// xoshiro256++ 1.0 (Blackman & Vigna, 2019)
// All-purpose 64-bit generator with period 2^256 - 1.
// Passes all BigCrush and PractRand tests.
class Xoshiro256PlusPlus
{
private:
    std::array<std::uint64_t, 4> s{0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL,
                                   0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL};

public:
    using result_type = std::uint64_t;

    static constexpr std::uint64_t default_seed = 0x544E4750524E47ULL;

    constexpr Xoshiro256PlusPlus() noexcept = default;

    constexpr explicit Xoshiro256PlusPlus(std::uint64_t seed_val) noexcept
    {
        seed(seed_val);
    }

    constexpr Xoshiro256PlusPlus(std::uint64_t s0, std::uint64_t s1, std::uint64_t s2, std::uint64_t s3) noexcept
        : s{s0, s1, s2, s3}
    {
        if ((s[0] | s[1] | s[2] | s[3]) == 0) {
            seed(default_seed);
        }
    }

    static constexpr result_type min() noexcept
    {
        return 0;
    }

    static constexpr result_type max() noexcept
    {
        return std::numeric_limits<result_type>::max();
    }

    constexpr void seed(std::uint64_t seed_val = default_seed) noexcept
    {
        SplitMix64 sm(seed_val);
        s[0] = sm.next();
        s[1] = sm.next();
        s[2] = sm.next();
        s[3] = sm.next();
    }

    constexpr result_type operator()() noexcept
    {
        return next();
    }

    constexpr std::uint64_t next() noexcept
    {
        const std::uint64_t result = rotl(s[0] + s[3], 23) + s[0];

        const std::uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;

        s[3] = rotl(s[3], 45);

        return result;
    }

    // Equivalent to 2^128 calls to next(); can be used to generate
    // 2^128 non-overlapping subsequences for parallel computation.
    constexpr void jump() noexcept
    {
        constexpr std::array<std::uint64_t, 4> JUMP = {
            0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL,
            0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL
        };

        std::uint64_t s0 = 0;
        std::uint64_t s1 = 0;
        std::uint64_t s2 = 0;
        std::uint64_t s3 = 0;

        for (std::size_t i = 0; i < JUMP.size(); ++i) {
            for (int b = 0; b < 64; ++b) {
                if ((JUMP[i] & (1ULL << b)) != 0) {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                next();
            }
        }

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }

    // Equivalent to 2^192 calls to next(); can be used to generate
    // 2^64 starting points, each generating 2^64 non-overlapping subsequences.
    constexpr void long_jump() noexcept
    {
        constexpr std::array<std::uint64_t, 4> LONG_JUMP = {
            0x76e15d3efefdcbbfULL, 0xc5004e441c522fb3ULL,
            0x77710069854ee241ULL, 0x39109bb02acbe635ULL
        };

        std::uint64_t s0 = 0;
        std::uint64_t s1 = 0;
        std::uint64_t s2 = 0;
        std::uint64_t s3 = 0;

        for (std::size_t i = 0; i < LONG_JUMP.size(); ++i) {
            for (int b = 0; b < 64; ++b) {
                if ((LONG_JUMP[i] & (1ULL << b)) != 0) {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                next();
            }
        }

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }

    [[nodiscard]] constexpr const std::array<std::uint64_t, 4>& getState() const noexcept
    {
        return s;
    }

    constexpr bool operator==(const Xoshiro256PlusPlus&) const noexcept = default;

    friend std::ostream& operator<<(std::ostream& os, const Xoshiro256PlusPlus& rng)
    {
        return os << rng.s[0] << ' ' << rng.s[1] << ' ' << rng.s[2] << ' ' << rng.s[3];
    }

    friend std::istream& operator>>(std::istream& is, Xoshiro256PlusPlus& rng)
    {
        std::uint64_t s0{}, s1{}, s2{}, s3{};
        if (is >> s0 >> s1 >> s2 >> s3) {
            if ((s0 | s1 | s2 | s3) != 0) {
                rng.s[0] = s0;
                rng.s[1] = s1;
                rng.s[2] = s2;
                rng.s[3] = s3;
            } else {
                is.setstate(std::ios_base::failbit);
            }
        }
        return is;
    }
};

// xoshiro256** 1.0 (Blackman & Vigna, 2018)
// StarStar variant
class Xoshiro256StarStar
{
private:
    std::array<std::uint64_t, 4> s{0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL,
                                   0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL};

public:
    using result_type = std::uint64_t;

    static constexpr std::uint64_t default_seed = 0x544E4750524E47ULL;

    constexpr Xoshiro256StarStar() noexcept = default;

    constexpr explicit Xoshiro256StarStar(std::uint64_t seed_val) noexcept
    {
        seed(seed_val);
    }

    constexpr Xoshiro256StarStar(std::uint64_t s0, std::uint64_t s1, std::uint64_t s2, std::uint64_t s3) noexcept
        : s{s0, s1, s2, s3}
    {
        if ((s[0] | s[1] | s[2] | s[3]) == 0) {
            seed(default_seed);
        }
    }

    static constexpr result_type min() noexcept
    {
        return 0;
    }

    static constexpr result_type max() noexcept
    {
        return std::numeric_limits<result_type>::max();
    }

    constexpr void seed(std::uint64_t seed_val = default_seed) noexcept
    {
        SplitMix64 sm(seed_val);
        s[0] = sm.next();
        s[1] = sm.next();
        s[2] = sm.next();
        s[3] = sm.next();
    }

    constexpr result_type operator()() noexcept
    {
        return next();
    }

    constexpr std::uint64_t next() noexcept
    {
        const std::uint64_t result = rotl(s[1] * 5, 7) * 9;

        const std::uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;

        s[3] = rotl(s[3], 45);

        return result;
    }

    constexpr void jump() noexcept
    {
        constexpr std::array<std::uint64_t, 4> JUMP = {
            0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL,
            0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL
        };

        std::uint64_t s0 = 0;
        std::uint64_t s1 = 0;
        std::uint64_t s2 = 0;
        std::uint64_t s3 = 0;

        for (std::size_t i = 0; i < JUMP.size(); ++i) {
            for (int b = 0; b < 64; ++b) {
                if ((JUMP[i] & (1ULL << b)) != 0) {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                next();
            }
        }

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }

    constexpr void long_jump() noexcept
    {
        constexpr std::array<std::uint64_t, 4> LONG_JUMP = {
            0x76e15d3efefdcbbfULL, 0xc5004e441c522fb3ULL,
            0x77710069854ee241ULL, 0x39109bb02acbe635ULL
        };

        std::uint64_t s0 = 0;
        std::uint64_t s1 = 0;
        std::uint64_t s2 = 0;
        std::uint64_t s3 = 0;

        for (std::size_t i = 0; i < LONG_JUMP.size(); ++i) {
            for (int b = 0; b < 64; ++b) {
                if ((LONG_JUMP[i] & (1ULL << b)) != 0) {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                next();
            }
        }

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }

    [[nodiscard]] constexpr const std::array<std::uint64_t, 4>& getState() const noexcept
    {
        return s;
    }

    constexpr bool operator==(const Xoshiro256StarStar&) const noexcept = default;

    friend std::ostream& operator<<(std::ostream& os, const Xoshiro256StarStar& rng)
    {
        return os << rng.s[0] << ' ' << rng.s[1] << ' ' << rng.s[2] << ' ' << rng.s[3];
    }

    friend std::istream& operator>>(std::istream& is, Xoshiro256StarStar& rng)
    {
        std::uint64_t s0{}, s1{}, s2{}, s3{};
        if (is >> s0 >> s1 >> s2 >> s3) {
            if ((s0 | s1 | s2 | s3) != 0) {
                rng.s[0] = s0;
                rng.s[1] = s1;
                rng.s[2] = s2;
                rng.s[3] = s3;
            } else {
                is.setstate(std::ios_base::failbit);
            }
        }
        return is;
    }
};

using Xoshiro256 = Xoshiro256PlusPlus;

} // namespace RL

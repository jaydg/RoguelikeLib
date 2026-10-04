//////////////////////////////////////////////////////////////////////////
// Clock
//
// The time of day in a world where every turn takes some minutes, and
// what is meant to happen at what time of day: the phases of the day, the
// routine of a character.
//
// RL::CClock clock(1, 8);                   // a minute a turn, from 8:00
//
// RL::CDailySchedule<std::string_view> day;
// day.Set(6, 0, "morning");
// day.Set(12, 0, "afternoon");
// day.Set(18, 0, "evening");
// day.Set(22, 0, "night");
//
// clock.Advance(300);
// day.At(clock);                            // "afternoon", at 13:00
//////////////////////////////////////////////////////////////////////////

module;

export module rl.clock;

import std;

export namespace RL
{

constexpr unsigned minutes_per_hour = 60;
constexpr unsigned minutes_per_day = 24 * minutes_per_hour;

class CClock
{
private:
    // Minutes since midnight of the first day
    std::uint64_t minutes;
    unsigned minutes_per_turn;

public:
    // A clock that is advanced `a_minutes_per_turn` minutes a turn, and
    // starts on the first day at the hour and minute given
    explicit CClock(unsigned a_minutes_per_turn = 1, unsigned hour = 0, unsigned minute = 0)
        : minutes(hour * std::uint64_t{minutes_per_hour} + minute), minutes_per_turn(a_minutes_per_turn)
    {
        if (hour >= 24 || minute >= minutes_per_hour) {
            throw std::invalid_argument(std::format("{}:{:02} is not a time of day", hour, minute));
        }
    }

    // Lets turns pass
    void Advance(unsigned turns = 1)
    {
        minutes += std::uint64_t{turns} * minutes_per_turn;
    }

    [[nodiscard]] unsigned MinutesPerTurn() const
    {
        return minutes_per_turn;
    }

    // Days passed since the clock started: 0 on the first day
    [[nodiscard]] std::uint64_t Day() const
    {
        return minutes / minutes_per_day;
    }

    // Minutes since midnight, 0 to 1439
    [[nodiscard]] unsigned MinuteOfDay() const
    {
        return static_cast<unsigned>(minutes % minutes_per_day);
    }

    [[nodiscard]] unsigned Hour() const
    {
        return MinuteOfDay() / minutes_per_hour;
    }

    [[nodiscard]] unsigned Minute() const
    {
        return MinuteOfDay() % minutes_per_hour;
    }

    // The time of day, like "08:05"
    [[nodiscard]] std::string ToString() const
    {
        return std::format("{:02}:{:02}", Hour(), Minute());
    }
};

// What holds at what time of day. Each entry holds from its time of day
// on, until the next entry's; the last one holds past midnight, until the
// first one of the next day.
template <typename T>
class CDailySchedule
{
private:
    // Sorted by the minute of the day they start at
    std::vector<std::pair<unsigned, T>> entries;

public:
    // From this time of day on, the schedule says `value`. An entry for the
    // same time is replaced.
    void Set(unsigned hour, unsigned minute, T value)
    {
        if (hour >= 24 || minute >= minutes_per_hour) {
            throw std::invalid_argument(std::format("{}:{:02} is not a time of day", hour, minute));
        }

        const unsigned start = hour * minutes_per_hour + minute;
        const auto it = std::ranges::lower_bound(entries, start, {}, &std::pair<unsigned, T>::first);

        if (it != entries.end() && it->first == start) {
            it->second = std::move(value);
        } else {
            entries.insert(it, {start, std::move(value)});
        }
    }

    [[nodiscard]] bool Empty() const
    {
        return entries.empty();
    }

    // What holds at a minute of the day, 0 to 1439
    [[nodiscard]] const T& At(unsigned minute_of_day) const
    {
        if (entries.empty()) {
            throw std::logic_error("an empty daily schedule says nothing");
        }

        const unsigned minute = minute_of_day % minutes_per_day;
        const auto it = std::ranges::upper_bound(entries, minute, {}, &std::pair<unsigned, T>::first);

        // Before the first entry of the day, the last one still holds
        return it == entries.begin() ? entries.back().second : std::prev(it)->second;
    }

    // What holds at the clock's time
    [[nodiscard]] const T& At(const CClock& clock) const
    {
        return At(clock.MinuteOfDay());
    }
};

} // namespace RL

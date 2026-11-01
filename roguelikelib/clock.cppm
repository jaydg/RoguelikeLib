/**
 * @file clock.cppm
 * @brief Turn-based game clock, time of day tracking, and daily recurring schedules.
 *
 * Provides @ref RL::CClock for advancing game time by a fixed number of minutes
 * per turn and querying the time of day, elapsed days, and formatted timestamps,
 * as well as @ref RL::CDailySchedule for modeling recurring daily phases or NPC routines.
 */

module;

export module rl.clock;

import std;

export namespace RL
{

/// Number of minutes in one standard hour.
constexpr unsigned minutes_per_hour = 60;

/// Number of minutes in one standard 24-hour day (1440 minutes).
constexpr unsigned minutes_per_day = 24 * minutes_per_hour;

/**
 * @brief Turn-based game clock tracking elapsed minutes, time of day, and days.
 *
 * @ref CClock models in-game time in worlds where turns advance by a configured number
 * of minutes (@ref MinutesPerTurn). It computes the 24-hour time of day (@ref Hour, @ref Minute),
 * elapsed days (@ref Day), and total minutes elapsed since world creation (@ref Minutes).
 *
 * The elapsed day count returned by @ref Day() directly integrates with @ref RL::CCalendar::DateOf()
 * to map turn progression into calendar years, months, and dates.
 *
 * ### Key Features
 * - **Configurable Turn Duration:** Set how many in-game minutes pass per turn (e.g., 1 min for tactical movement, 10-30 mins for overland travel).
 * - **Modular Time Queries:** Query hour (0-23), minute (0-59), minute of day (0-1439), and day count (0 on day 1).
 * - **Serialization Support:** Easily serialize via @ref Minutes and restore via @ref FromMinutes.
 * - **Formatted Output:** Produces 24-hour `"HH:MM"` timestamp strings via @ref ToString.
 *
 * ### Usage Example
 * @code
 * import rl.clock;
 *
 * // Create a clock advancing 5 minutes per turn, starting at 08:00
 * RL::CClock clock(5, 8, 0);
 *
 * // Advance 12 turns (60 minutes)
 * clock.Advance(12);
 *
 * clock.ToString(); // "09:00"
 * clock.Hour();     // 9
 * clock.Minute();   // 0
 * clock.Day();      // 0 (still day 1)
 * @endcode
 *
 * @see CDailySchedule
 * @see CCalendar
 */
class CClock
{
private:
    /// Total minutes elapsed since midnight of day 0.
    std::uint64_t minutes;

    /// In-game minutes elapsed per turn.
    unsigned minutes_per_turn;

public:
    /**
     * @brief Constructs a clock starting at a given time of day.
     *
     * @param a_minutes_per_turn In-game minutes that pass per call to @ref Advance (defaults to 1).
     * @param hour Initial hour of the day (`0` to `23`, defaults to 0).
     * @param minute Initial minute of the hour (`0` to `59`, defaults to 0).
     * @throws std::invalid_argument If @p hour >= 24 or @p minute >= 60.
     */
    explicit CClock(unsigned a_minutes_per_turn = 1, unsigned hour = 0, unsigned minute = 0)
        : minutes(hour * std::uint64_t{minutes_per_hour} + minute), minutes_per_turn(a_minutes_per_turn)
    {
        if (hour >= 24 || minute >= minutes_per_hour) {
            throw std::invalid_argument(std::format("{}:{:02} is not a time of day", hour, minute));
        }
    }

    /**
     * @brief Reconstructs a clock from a total elapsed minutes counter.
     *
     * Useful for restoring saved game states where the total running time was
     * serialized with @ref Minutes().
     *
     * @param a_minutes_per_turn In-game minutes per turn.
     * @param a_minutes Total elapsed minutes since midnight of day 0.
     * @return Initialized @ref CClock instance.
     */
    static CClock FromMinutes(unsigned a_minutes_per_turn, std::uint64_t a_minutes)
    {
        CClock clock(a_minutes_per_turn);
        clock.minutes = a_minutes;
        return clock;
    }

    /**
     * @brief Advances the clock by a given number of game turns.
     *
     * Adds `turns * minutes_per_turn` minutes to the elapsed time.
     *
     * @param turns Number of turns to advance (defaults to 1).
     */
    void Advance(unsigned turns = 1)
    {
        minutes += std::uint64_t{turns} * minutes_per_turn;
    }

    /**
     * @brief Returns total minutes elapsed since midnight of day 0.
     * @return Cumulative elapsed minutes.
     */
    [[nodiscard]] std::uint64_t Minutes() const
    {
        return minutes;
    }

    /**
     * @brief Returns the configured number of minutes that pass per turn.
     * @return Minutes per turn.
     */
    [[nodiscard]] unsigned MinutesPerTurn() const
    {
        return minutes_per_turn;
    }

    /**
     * @brief Returns the number of complete days elapsed since the clock began.
     *
     * Returns 0 during the entire first day (the first 1440 minutes).
     *
     * @return 0-based elapsed day count (`minutes / 1440`).
     */
    [[nodiscard]] std::uint64_t Day() const
    {
        return minutes / minutes_per_day;
    }

    /**
     * @brief Returns the current minute within the 24-hour day.
     * @return Minute of the day (`0` to `1439`).
     */
    [[nodiscard]] unsigned MinuteOfDay() const
    {
        return static_cast<unsigned>(minutes % minutes_per_day);
    }

    /**
     * @brief Returns the current hour of the day.
     * @return Hour of the day (`0` to `23`).
     */
    [[nodiscard]] unsigned Hour() const
    {
        return MinuteOfDay() / minutes_per_hour;
    }

    /**
     * @brief Returns the current minute of the hour.
     * @return Minute of the hour (`0` to `59`).
     */
    [[nodiscard]] unsigned Minute() const
    {
        return MinuteOfDay() % minutes_per_hour;
    }

    /**
     * @brief Formats the current time of day as a standard 24-hour string.
     * @return String formatted as `"HH:MM"` (e.g. `"08:05"`).
     */
    [[nodiscard]] std::string ToString() const
    {
        return std::format("{:02}:{:02}", Hour(), Minute());
    }
};

/**
 * @brief Recurring daily schedule mapping times of day to active states or phases.
 *
 * @ref CDailySchedule manages a timeline of daily events or states (e.g., day/night
 * lighting phases, monster spawning periods, or character daily routines).
 *
 * Each entry holds starting at its designated time of day and remains active until
 * the start of the next entry. The last entry of the day automatically wraps around
 * midnight and holds until the first entry of the next day.
 *
 * @tparam T Type of state value stored (e.g. `std::string`, `std::string_view`, `enum`).
 *
 * ### Usage Example
 * @code
 * import rl.clock;
 *
 * RL::CDailySchedule<std::string_view> phases;
 * phases.Set(6, 0, "morning");
 * phases.Set(12, 0, "afternoon");
 * phases.Set(18, 0, "evening");
 * phases.Set(22, 0, "night");
 *
 * RL::CClock clock(1, 14, 30);
 * phases.At(clock); // "afternoon"
 * @endcode
 *
 * @see CClock
 */
template <typename T>
class CDailySchedule
{
private:
    /// Sorted list of daily entries as (minute_of_day, value) pairs.
    std::vector<std::pair<unsigned, T>> entries;

public:
    /**
     * @brief Schedules a state value to begin at the specified time of day.
     *
     * The value remains active until the next scheduled entry. If an entry already
     * exists at the exact same hour and minute, its value is replaced.
     *
     * @param hour Hour of the day (`0` to `23`).
     * @param minute Minute of the hour (`0` to `59`).
     * @param value State value to activate at this time.
     * @throws std::invalid_argument If @p hour >= 24 or @p minute >= 60.
     */
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

    /**
     * @brief Checks if the schedule contains no entries.
     * @return `true` if empty, `false` otherwise.
     */
    [[nodiscard]] bool Empty() const
    {
        return entries.empty();
    }

    /**
     * @brief Queries the active value at a specific minute of the day.
     *
     * Automatically handles wrap-around past midnight: if @p minute_of_day precedes
     * the day's first entry, the last entry of the day is returned.
     *
     * @param minute_of_day Minute of the day (`0` to `1439`, modulo 1440 applied).
     * @return Const reference to the active state value.
     * @throws std::logic_error If the schedule is empty.
     */
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

    /**
     * @brief Queries the active value corresponding to the clock's current time of day.
     *
     * Convenience overload equivalent to calling `At(clock.MinuteOfDay())`.
     *
     * @param clock The game clock to query.
     * @return Const reference to the active state value.
     * @throws std::logic_error If the schedule is empty.
     */
    [[nodiscard]] const T& At(const CClock& clock) const
    {
        return At(clock.MinuteOfDay());
    }
};

} // namespace RL

/**
 * @file calendar.cppm
 * @brief In-game calendar system with custom year divisions, date mapping, and ordinal formatting.
 *
 * Provides @ref RL::CCalendar for tracking multi-month or multi-season annual cycles
 * in game worlds, converting sequential day counters (such as those from @ref RL::CClock::Day())
 * into structured dates (@ref RL::SDate), and formatting day numbers with English
 * ordinal suffixes (@ref RL::Ordinal).
 */

module;

export module rl.calendar;

import std;

export namespace RL
{

/**
 * @brief A named segment of a calendar year (e.g., month, moon cycle, or season).
 *
 * Each calendar part defines a display name and a fixed duration in days.
 * A year is formed by an ordered sequence of one or more such parts.
 */
struct SCalendarPart {
    std::string name;   ///< Display name of the part (e.g., `"Thawmoon"`, `"January"`, `"Harvest"`).
    unsigned days = 0;  ///< Duration of this part in days (must contribute to a non-zero total year length).
};

/**
 * @brief A human-readable calendar date comprising year, part index, and day of part.
 *
 * Represents a calendar date broken down into:
 * - `year`: 1-based year number (starts at 1).
 * - `part`: 0-based index into the calendar's parts list (`0` to `CCalendar::Parts() - 1`).
 * - `day`: 1-based day within the current part (`1` to `CCalendar::PartDays(part)`).
 */
struct SDate {
    std::uint64_t year = 1; ///< 1-based year count (starts at 1).
    std::size_t part = 0;   ///< 0-based index of the current calendar part / month.
    unsigned day = 1;       ///< 1-based day of the month/part (starts at 1).

    /**
     * @brief Equality comparison for calendar dates.
     */
    bool operator==(const SDate&) const = default;
};

/**
 * @brief Flexible calendar system for game worlds with arbitrary months, seasons, and years.
 *
 * @ref CCalendar models an annual cycle composed of ordered parts (@ref SCalendarPart),
 * such as months, moon phases, or festival periods. It translates linear elapsed day counts
 * (e.g. from an in-game time or turn simulation) into structured @ref SDate representations
 * (Year, Month/Part, Day of Month), taking an optional initial day-of-year offset into account.
 *
 * ### Key Features
 * - **Arbitrary Year Structure:** Custom number of months/parts with arbitrary, variable lengths.
 * - **Flexible Epoch Offset:** Can begin counting from any arbitrary day of the year (`a_first_day`).
 * - **Bidirectional Mapping:** Converts linear day counts to @ref SDate via @ref DateOf, and queries
 *   day-of-year positions via @ref DayOfYear and @ref FirstDayOf.
 * - **Seamless Clock Integration:** Directly consumes the day count produced by @ref RL::CClock::Day().
 *
 * ### Usage Example
 * @code
 * import rl.calendar;
 * import rl.clock;
 *
 * // Define a 4-season fantasy calendar starting on day 10 of the year
 * RL::CCalendar calendar({
 *     {"Thawmoon", 30},
 *     {"Seedmoon", 30},
 *     {"Sunmoon", 30},
 *     {"Frostmoon", 30}
 * }, 10);
 *
 * // Convert elapsed day 0 to its in-game date
 * RL::SDate date = calendar.DateOf(0);
 * // date.year == 1, date.part == 0 ("Thawmoon"), date.day == 11
 *
 * // After 40 days:
 * RL::SDate later = calendar.DateOf(40);
 * // later.year == 1, later.part == 1 ("Seedmoon"), later.day == 21
 *
 * // Format string:
 * std::string formatted = std::format("Year {}, {} of {}",
 *     later.year, RL::Ordinal(later.day), calendar.PartName(later.part));
 * // "Year 1, 21st of Seedmoon"
 * @endcode
 *
 * @see SCalendarPart
 * @see SDate
 * @see Ordinal
 */
class CCalendar
{
private:
    std::vector<SCalendarPart> parts;
    unsigned days_per_year = 0;

    /// The day of the first year on which elapsed day 0 falls (0-indexed, normalized).
    std::uint64_t first_day = 0;

public:
    /**
     * @brief Constructs a calendar with the specified ordered parts and initial day offset.
     *
     * Computes the total days per year by summing the durations of all parts.
     * The initial day offset @p a_first_day specifies which day of the annual cycle
     * elapsed day 0 corresponds to, normalized via modulo @ref DaysPerYear().
     *
     * @param a_parts Ordered vector of calendar parts (months/seasons) defining the year.
     * @param a_first_day 0-based day of the year corresponding to elapsed day 0 (defaults to 0).
     * @throws std::invalid_argument If the total days across all parts is zero, or if @p a_parts is empty.
     */
    explicit CCalendar(std::vector<SCalendarPart> a_parts, std::uint64_t a_first_day = 0)
        : parts(std::move(a_parts))
    {
        for (const SCalendarPart& part : parts) {
            days_per_year += part.days;
        }

        if (days_per_year == 0) {
            throw std::invalid_argument("a year of no days is no calendar");
        }

        first_day = a_first_day % days_per_year;
    }

    /**
     * @brief Returns the total number of days in one complete annual cycle.
     * @return Sum of days of all configured calendar parts.
     */
    [[nodiscard]] unsigned DaysPerYear() const
    {
        return days_per_year;
    }

    /**
     * @brief Returns the number of parts (months/seasons) in the calendar year.
     * @return Number of entries in the parts list.
     */
    [[nodiscard]] std::size_t Parts() const
    {
        return parts.size();
    }

    /**
     * @brief Retrieves the display name of a calendar part by its 0-based index.
     * @param part 0-based index of the part (`0` to `Parts() - 1`).
     * @return Const reference to the part's name string.
     * @throws std::out_of_range If @p part is greater than or equal to @ref Parts().
     */
    [[nodiscard]] const std::string& PartName(std::size_t part) const
    {
        return parts.at(part).name;
    }

    /**
     * @brief Retrieves the duration in days of a calendar part by its 0-based index.
     * @param part 0-based index of the part (`0` to `Parts() - 1`).
     * @return Number of days in the specified part.
     * @throws std::out_of_range If @p part is greater than or equal to @ref Parts().
     */
    [[nodiscard]] unsigned PartDays(std::size_t part) const
    {
        return parts.at(part).days;
    }

    /**
     * @brief Computes the 0-based day of the year for a given elapsed day count.
     *
     * Accounts for the initial @p first_day offset passed during construction.
     *
     * @param day Linear elapsed day counter (e.g. 0 on world creation day).
     * @return 0-based day of the year (`0` to `DaysPerYear() - 1`).
     */
    [[nodiscard]] unsigned DayOfYear(std::uint64_t day) const
    {
        return static_cast<unsigned>((first_day + day) % days_per_year);
    }

    /**
     * @brief Converts a linear elapsed day counter into a structured calendar date.
     *
     * Calculates the 1-based year, the 0-based part (month) index, and the 1-based day
     * within that part, advancing through years as the cycle repeats.
     *
     * @param day Linear elapsed day counter (0 for the starting day).
     * @return @ref SDate containing year (1-based), part index (0-based), and day (1-based).
     */
    [[nodiscard]] SDate DateOf(std::uint64_t day) const
    {
        const std::uint64_t since_year_began = first_day + day;
        SDate date;
        date.year = 1 + since_year_began / days_per_year;
        unsigned of_year = static_cast<unsigned>(since_year_began % days_per_year);

        for (std::size_t part = 0; part < parts.size(); ++part) {
            if (of_year < parts[part].days) {
                date.part = part;
                date.day = of_year + 1;
                break;
            }

            of_year -= parts[part].days;
        }

        return date;
    }

    /**
     * @brief Computes the 0-based day of the year on which a given calendar part begins.
     *
     * Sums the days of all preceding parts (`0` through `part - 1`).
     * For part 0, returns 0.
     *
     * @param part 0-based index of the part.
     * @return 0-based start day of the part within the year (`0` to `DaysPerYear() - 1`).
     */
    [[nodiscard]] unsigned FirstDayOf(std::size_t part) const
    {
        unsigned day = 0;

        for (std::size_t before = 0; before < part && before < parts.size(); ++before) {
            day += parts[before].days;
        }

        return day;
    }
};

/**
 * @brief Formats an integer with its English ordinal suffix (e.g., `"1st"`, `"2nd"`, `"3rd"`, `"4th"`).
 *
 * Correctly handles irregular teen numbers (`"11th"`, `"12th"`, `"13th"`), as well as
 * standard endings for `"st"`, `"nd"`, `"rd"`, and `"th"`. Useful for formatting
 * calendar days (e.g. `Ordinal(date.day)` -> `"21st"`).
 *
 * @param number Number to format.
 * @return Formatted string including the number and its ordinal suffix.
 */
[[nodiscard]] std::string Ordinal(std::uint64_t number)
{
    const std::uint64_t tens = number % 100;
    const std::uint64_t ones = number % 10;
    const std::string_view ending = tens >= 11 && tens <= 13 ? "th" : ones == 1 ? "st" : ones == 2 ? "nd" : ones == 3 ? "rd" : "th";
    return std::to_string(number) + std::string(ending);
}

} // namespace RL

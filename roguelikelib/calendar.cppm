//////////////////////////////////////////////////////////////////////////
// Calendar
//
// The days of a year in parts - months, say - each with a name and so many
// days, the year going round; and the date of a day counted from a first
// day, which falls on any day of the year the calendar is told.
//
// RL::CCalendar calendar({{"Thawmoon", 10}, {"Seedmoon", 10}}, 10);
// const RL::SDate date = calendar.DateOf(3);  // year 1, Seedmoon, day 4
// calendar.PartName(date.part);              // "Seedmoon"
//////////////////////////////////////////////////////////////////////////

module;

export module rl.calendar;

import std;

export namespace RL
{

// A part of the year, like a month: its name, and how many days it has
struct SCalendarPart {
    std::string name;
    unsigned days = 0;
};

// A date: the year, from 1; the part of the year, by its index; and the
// day of the part, from 1
struct SDate {
    std::uint64_t year = 1;
    std::size_t part = 0;
    unsigned day = 1;

    bool operator==(const SDate&) const = default;
};

class CCalendar
{
private:
    std::vector<SCalendarPart> parts;
    unsigned days_per_year = 0;

    // The day of the first year the first day counted falls on, from 0
    std::uint64_t first_day = 0;

public:
    // A calendar of the parts given, in order, whose first day counted is
    // the day of the year given, from 0. A year needs a day at least.
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

    [[nodiscard]] unsigned DaysPerYear() const
    {
        return days_per_year;
    }

    [[nodiscard]] std::size_t Parts() const
    {
        return parts.size();
    }

    [[nodiscard]] const std::string& PartName(std::size_t part) const
    {
        return parts.at(part).name;
    }

    [[nodiscard]] unsigned PartDays(std::size_t part) const
    {
        return parts.at(part).days;
    }

    // The day of the year a day counted falls on, from 0
    [[nodiscard]] unsigned DayOfYear(std::uint64_t day) const
    {
        return static_cast<unsigned>((first_day + day) % days_per_year);
    }

    // The date of a day counted, from 0: the first day, as the calendar was
    // told, then on
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

    // The day of the year a date's part begins on, from 0
    [[nodiscard]] unsigned FirstDayOf(std::size_t part) const
    {
        unsigned day = 0;

        for (std::size_t before = 0; before < part && before < parts.size(); ++before) {
            day += parts[before].days;
        }

        return day;
    }
};

// A number as one says it of a day: "1st", "2nd", "3rd", "4th", "11th",
// "22nd"
[[nodiscard]] std::string Ordinal(std::uint64_t number)
{
    const std::uint64_t tens = number % 100;
    const std::uint64_t ones = number % 10;
    const std::string_view ending = tens >= 11 && tens <= 13 ? "th" : ones == 1 ? "st" : ones == 2 ? "nd" : ones == 3 ? "rd" : "th";
    return std::to_string(number) + std::string(ending);
}

} // namespace RL

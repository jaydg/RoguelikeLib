// Tests for RoguelikeLib's rl.calendar: parts of the year, and the date of
// a day

import rl.calendar;
import rltests.check;
import std;

using RL::tests::Check;

void TestCalendar()
{
    // The library's calendar: parts of the year, and the date of a day
    {
        const RL::CCalendar calendar({{"Thawmoon", 10}, {"Seedmoon", 10}, {"Haymoon", 5}}, 12);
        Check(calendar.DaysPerYear() == 25 && calendar.Parts() == 3 && calendar.PartName(1) == "Seedmoon",
              "a calendar has the parts of a year it was given");
        Check(calendar.DateOf(0) == RL::SDate{1, 1, 3} && calendar.DateOf(8) == RL::SDate{1, 2, 1} &&
              calendar.DateOf(13) == RL::SDate{2, 0, 1}, "the first day counted falls where it was told, and the year goes round");
        Check(calendar.DayOfYear(13) == 0 && calendar.FirstDayOf(2) == 20, "and knows the day of the year");
        Check(RL::Ordinal(1) == "1st" && RL::Ordinal(2) == "2nd" && RL::Ordinal(3) == "3rd" && RL::Ordinal(4) == "4th" &&
              RL::Ordinal(11) == "11th" && RL::Ordinal(12) == "12th" && RL::Ordinal(22) == "22nd", "days are told as one says them");

        bool refused = false;

        try {
            const RL::CCalendar none({});
        } catch (const std::invalid_argument&) {
            refused = true;
        }

        Check(refused, "a year of no days is no calendar");
    }
}

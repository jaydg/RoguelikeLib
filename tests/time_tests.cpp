// Tests for RoguelikeLib's rl.scheduler and rl.clock

import rl.clock;
import rl.scheduler;
import rltests.check;
import std;

using RL::tests::Check;

void TestScheduler()
{
    RL::CScheduler<char> scheduler;
    Check(!scheduler.Next().has_value() && scheduler.Empty() && scheduler.Now() == 0, "an empty timeline hands out no one");

    // A quick, an ordinary and a slow actor, each acting again after its
    // own delay. Actors due at once act in the order they were scheduled.
    const std::map<char, RL::Ticks> delay = {{'A', 50}, {'B', 100}, {'C', 200}};

    for (const char actor : {
                'A', 'B', 'C'
            }) {
        scheduler.Schedule(actor, 0);
    }

    std::string turns;
    std::vector<RL::Ticks> times;

    for (int turn = 0; turn < 10; ++turn) {
        const auto actor = scheduler.Next();
        turns += *actor;
        times.push_back(scheduler.Now());
        scheduler.Schedule(*actor, delay.at(*actor));
    }

    Check(turns == "ABCABAACBA", "quicker actors act more often, and ties in the order of scheduling");
    Check(times == std::vector<RL::Ticks> {0, 0, 0, 50, 100, 100, 150, 200, 200, 200}, "the time moves on to each turn");
    Check(scheduler.Size() == 3 && scheduler.Contains('B'), "every actor is back on the timeline");

    // Removing
    scheduler.Remove('A');
    scheduler.Remove('X');
    std::string without;

    for (int turn = 0; turn < 4; ++turn) {
        const auto actor = scheduler.Next();
        without += *actor;
        scheduler.Schedule(*actor, delay.at(*actor));
    }

    Check(without.find('A') == std::string::npos && !scheduler.Contains('A') && scheduler.Size() == 2,
          "a removed actor acts no more; removing one not there does nothing");

    // Scheduling again moves an actor rather than adding a second turn
    RL::CScheduler<int> moved;
    moved.Schedule(1, 10);
    moved.Schedule(2, 7);
    moved.Schedule(1, 5);
    Check(moved.Next() == 1 && moved.Now() == 5 && moved.Next() == 2 && moved.Now() == 7 && !moved.Next().has_value(),
          "scheduling again moves the turn");

    // An actor acting again at once waits for those due before it
    RL::CScheduler<int> again;
    again.Schedule(1, 0);
    again.Schedule(2, 0);
    const int first = *again.Next();
    again.Schedule(first, 0);
    Check(first == 1 && again.Next() == 2 && again.Next() == 1, "acting again at once comes after the others due");

    // Pointers will do as identifiers
    int a = 0, b = 0;
    RL::CScheduler<int*> pointers;
    pointers.Schedule(&b, 2);
    pointers.Schedule(&a, 1);
    Check(pointers.Next() == &a && pointers.Next() == &b, "pointers as identifiers");

    // A timeline taken down and put back the same goes on the same
    RL::CScheduler<char> original;
    original.Schedule('A', 30);
    original.Schedule('B', 10);
    original.Schedule('C', 30);
    original.Schedule('D', 20);
    original.Remove('D');
    original.Schedule('B', 30);
    static_cast<void>(original.Next());

    const auto scheduled = original.Scheduled();
    Check(scheduled.size() == 2 && scheduled[0].id == 'C' && scheduled[1].id == 'B' && scheduled[1].time == 30,
          "the timeline lists who acts when, in order, and only those still on it");

    RL::CScheduler<char> restored;
    restored.Schedule('X', 0);
    restored.Restore(original.Now(), scheduled);
    Check(restored.Now() == original.Now() && restored.Size() == 2 && !restored.Contains('X'),
          "a restored timeline has those actors and no others");

    std::string acted, acted_again;

    for (int round = 0; round < 6; ++round) {
        const char next = *original.Next();
        acted += next;
        original.Schedule(next, 10);

        const char next_again = *restored.Next();
        acted_again += next_again;
        restored.Schedule(next_again, 10);
    }

    Check(acted == acted_again && original.Now() == restored.Now(), "and goes on as the original does");

    bool threw = false;

    try {
        const std::array<RL::CScheduler<char>::SScheduled, 1> past = {{{'P', 5}}};
        restored.Restore(10, past);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "no one is put back to act in the past");
}

void TestClock()
{
    RL::CClock clock(1, 8);
    Check(clock.ToString() == "08:00" && clock.Day() == 0 && clock.Hour() == 8 && clock.Minute() == 0, "the clock starts");

    clock.Advance(305);
    Check(clock.ToString() == "13:05" && clock.MinuteOfDay() == 13 * 60 + 5, "turns pass");

    clock.Advance(11 * 60 - 5);
    Check(clock.ToString() == "00:00" && clock.Day() == 1, "a new day begins at midnight");

    RL::CClock slow(5);
    slow.Advance(12);
    Check(slow.ToString() == "01:00" && slow.MinutesPerTurn() == 5, "turns of several minutes");

    const RL::CClock again = RL::CClock::FromMinutes(slow.MinutesPerTurn(), slow.Minutes());
    Check(slow.Minutes() == 60 && again.ToString() == "01:00" && again.MinutesPerTurn() == 5,
          "a clock is set back to the minutes it has run");

    bool threw = false;

    try {
        RL::CClock wrong(1, 24);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "24:00 is no time to start at");

    RL::CDailySchedule<std::string_view> day;
    Check(day.Empty(), "a new schedule is empty");

    threw = false;

    try {
        static_cast<void>(day.At(0));
    } catch (const std::logic_error&) {
        threw = true;
    }

    Check(threw, "an empty schedule says nothing");

    day.Set(12, 0, "afternoon");
    day.Set(6, 0, "morning");
    day.Set(22, 0, "night");
    day.Set(18, 0, "evening");

    Check(day.At(6 * 60) == "morning" && day.At(12 * 60 - 1) == "morning" && day.At(12 * 60) == "afternoon" &&
          day.At(21 * 60 + 59) == "evening" && day.At(23 * 60 + 59) == "night", "each entry holds until the next");
    Check(day.At(0) == "night" && day.At(5 * 60 + 59) == "night", "the last entry holds past midnight");
    Check(day.At(clock) == "night" && day.At(24 * 60 + 6 * 60) == "morning", "at the clock's time, and on any day");

    day.Set(6, 0, "dawn");
    Check(day.At(7 * 60) == "dawn", "an entry for the same time is replaced");
}

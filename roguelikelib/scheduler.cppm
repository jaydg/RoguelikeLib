//////////////////////////////////////////////////////////////////////////
// Scheduler
//
// Whose turn it is. Actors are put on a timeline at the time they are to
// act next; the scheduler hands out the earliest one and moves its clock
// on to that time. Actors that are quicker act sooner after their last
// action - how much sooner is the caller's business, by the delay it asks
// for.
//
// RL::CScheduler<CMonster*> scheduler;
// scheduler.Schedule(&player, 0);
//
// while (auto actor = scheduler.Next()) {
// const RL::Ticks cost = (*actor)->Act();
// scheduler.Schedule(*actor, cost * 100 / (*actor)->Speed());
// }
//
// The scheduler holds identifiers, not the actors themselves, so it owns
// nothing; anything it can order and copy will do, like a pointer or an
// index. Actors due at the same time act in the order they were scheduled,
// so the same calls always give the same turns.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.scheduler;

import std;

export namespace RL
{

// Time on the scheduler's timeline
using Ticks = std::uint64_t;

template <typename Id>
class CScheduler
{
private:
    struct SEntry {
        Ticks time;

        // In which order entries were made, to break ties
        std::uint64_t order;

        Id id;
    };

    // The earliest entry first, and of those due at once the first made
    struct SLater {
        bool operator()(const SEntry& a, const SEntry& b) const
        {
            return a.time != b.time ? a.time > b.time : a.order > b.order;
        }
    };

    std::priority_queue<SEntry, std::vector<SEntry>, SLater> timeline;

    // The entry that counts for each actor on the timeline. Entries made
    // before it, or for actors since removed, are skipped when they come up.
    std::map<Id, std::uint64_t> current;

    Ticks now = 0;
    std::uint64_t next_order = 0;

public:
    // Puts an actor on the timeline to act `delay` ticks from now. An actor
    // on it already is moved to that time.
    void Schedule(const Id& id, Ticks delay)
    {
        current[id] = next_order;
        timeline.push({now + delay, next_order, id});
        ++next_order;
    }

    // Takes an actor off the timeline; one that is not on it is ignored
    void Remove(const Id& id)
    {
        current.erase(id);
    }

    [[nodiscard]] bool Contains(const Id& id) const
    {
        return current.contains(id);
    }

    // How many actors are on the timeline
    [[nodiscard]] std::size_t Size() const
    {
        return current.size();
    }

    [[nodiscard]] bool Empty() const
    {
        return current.empty();
    }

    // The time now: when the actor handed out last is acting
    [[nodiscard]] Ticks Now() const
    {
        return now;
    }

    // Takes the actor whose turn is next off the timeline, and moves the
    // time on to its turn. Nothing when there is no one on the timeline.
    // To act again, an actor has to be scheduled again.
    std::optional<Id> Next()
    {
        while (!timeline.empty()) {
            const SEntry entry = timeline.top();
            timeline.pop();

            const auto it = current.find(entry.id);

            if (it == current.end() || it->second != entry.order) {
                continue;
            }

            current.erase(it);
            now = entry.time;

            return entry.id;
        }

        return std::nullopt;
    }
};

} // namespace RL

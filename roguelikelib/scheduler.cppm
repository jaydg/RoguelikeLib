/**
 * @file scheduler.cppm
 * @brief Discrete-event timeline scheduler for turn-based actor turns and speed systems.
 *
 * Provides @ref RL::CScheduler to schedule game actors or delayed events along
 * a continuous timeline of ticks, implementing speed systems where faster actors act
 * more frequently, with deterministic FIFO tie-breaking and complete serialization support.
 */

module;

export module rl.scheduler;

import std;

export namespace RL
{

/**
 * @brief Time unit on the scheduler's timeline.
 *
 * Represents discrete simulation ticks or elapsed action costs.
 */
using Ticks = std::uint64_t;

/**
 * @brief Deterministic priority-timeline scheduler for turn-based actor actions.
 *
 * @ref CScheduler implements a classic roguelike speed/energy turn manager. Actors
 * are placed on a timeline at the tick when they are next eligible to act. The scheduler
 * advances its internal time (@ref Now()) to each event and yields the next actor (@ref Next()).
 *
 * Actions cost a variable number of ticks depending on actor speed and action type
 * (e.g. `cost = base_cost * 100 / actor_speed`). Quicker actors incur shorter delays
 * and thus receive turns more frequently.
 *
 * ### Key Features
 * - **Deterministic Tie-Breaking:** When multiple actors are scheduled for the exact same
 *   timestamp, they act in the order in which they were scheduled (FIFO), guaranteeing 100%
 *   reproducible turn sequences.
 * - **Flexible Identity:** Identifies actors via @p Id handles (such as raw pointers, integer
 *   entity IDs, or string keys). The scheduler does not manage object ownership.
 * - **Dynamic Rescheduling:** Scheduling an actor that is already queued cleanly updates
 *   their target tick without duplicating entries or requiring linear queue searches.
 * - **Cancellation:** @ref Remove() cancels an actor's pending turn in @f$O(\log N)@f$ time.
 * - **Full Save/Restore Serialization:** @ref Scheduled() inspects all queued entities in chronological
 *   order, and @ref Restore() reconstructs the exact timeline state.
 *
 * @tparam Id Type of actor identifier. Must be copyable and support strict weak ordering
 *            via `std::less<Id>` (for storage in `std::map`).
 *
 * ### Usage Example
 * @code
 * import rl.scheduler;
 *
 * struct Actor {
 *     int speed = 100; // standard speed
 * };
 *
 * Actor player{.speed = 100};
 * Actor fast_monster{.speed = 200};
 *
 * RL::CScheduler<Actor*> timeline;
 * timeline.Schedule(&player, 0);
 * timeline.Schedule(&fast_monster, 0);
 *
 * while (auto actor = timeline.Next()) {
 *     // Perform turn...
 *     const RL::Ticks base_action_cost = 100;
 *     const RL::Ticks delay = base_action_cost * 100 / (*actor)->speed;
 *     timeline.Schedule(*actor, delay);
 *     break;
 * }
 * @endcode
 */
template <typename Id>
class CScheduler
{
private:
    struct SEntry {
        Ticks time;          ///< Target execution timestamp.
        std::uint64_t order; ///< Insertion sequence number for deterministic tie-breaking.
        Id id;               ///< Actor identifier.
    };

    struct SLater {
        bool operator()(const SEntry& a, const SEntry& b) const
        {
            return a.time != b.time ? a.time > b.time : a.order > b.order;
        }
    };

    std::priority_queue<SEntry, std::vector<SEntry>, SLater> timeline;

    /// Tracks the active generation order for each actor. Stale entries are discarded lazily.
    std::map<Id, std::uint64_t> current;

    Ticks now = 0;
    std::uint64_t next_order = 0;

public:
    /**
     * @brief A scheduled entity snapshot representing an actor and their target action tick.
     */
    struct SScheduled {
        Id id;      ///< Identifier of the scheduled entity.
        Ticks time; ///< Absolute timestamp at which the entity will act.
    };

    /**
     * @brief Schedules an actor to act after a delay from the current time.
     *
     * Places the actor on the timeline at `Now() + delay`. If the actor is already
     * on the timeline, its scheduled time is updated to the new timestamp.
     *
     * @param id Identifier of the entity to schedule.
     * @param delay Number of ticks from the current time (@ref Now()) until the turn occurs.
     */
    void Schedule(const Id& id, Ticks delay)
    {
        current[id] = next_order;
        timeline.push({now + delay, next_order, id});
        ++next_order;
    }

    /**
     * @brief Removes an actor from the timeline.
     *
     * If the actor is scheduled, its upcoming turn is cancelled. If the actor is not
     * currently on the timeline, this function has no effect.
     *
     * @param id Identifier of the entity to remove.
     */
    void Remove(const Id& id)
    {
        current.erase(id);
    }

    /**
     * @brief Checks if an actor currently has a pending turn on the timeline.
     * @param id Identifier of the entity.
     * @return `true` if scheduled, `false` otherwise.
     */
    [[nodiscard]] bool Contains(const Id& id) const
    {
        return current.contains(id);
    }

    /**
     * @brief Returns the number of distinct actors currently scheduled on the timeline.
     * @return Number of active scheduled entities.
     */
    [[nodiscard]] std::size_t Size() const
    {
        return current.size();
    }

    /**
     * @brief Checks whether the timeline contains no scheduled actors.
     * @return `true` if no actors are scheduled, `false` otherwise.
     */
    [[nodiscard]] bool Empty() const
    {
        return current.empty();
    }

    /**
     * @brief Returns the current timeline timestamp.
     *
     * Represents the tick associated with the actor yielded by the most recent call
     * to @ref Next(). Starts at 0.
     *
     * @return Current tick count.
     */
    [[nodiscard]] Ticks Now() const
    {
        return now;
    }

    /**
     * @brief Retrieves all scheduled actors in the chronological order they will act.
     *
     * Stale or cancelled entries are omitted. Useful for serializing the scheduler
     * state to disk or displaying turn initiative queues.
     *
     * @return Vector of @ref SScheduled snapshots ordered by execution time.
     */
    [[nodiscard]] std::vector<SScheduled> Scheduled() const
    {
        auto pending = timeline;
        std::vector<SScheduled> scheduled;

        while (!pending.empty()) {
            const SEntry entry = pending.top();
            pending.pop();

            if (const auto it = current.find(entry.id); it != current.end() && it->second == entry.order) {
                scheduled.push_back({entry.id, entry.time});
            }
        }

        return scheduled;
    }

    /**
     * @brief Restores the timeline state from a saved snapshot.
     *
     * Clears all existing scheduled entries and sets the timeline's clock to @p a_now.
     * Re-inserts the given actors in the specified sequence.
     *
     * @param a_now The current simulation tick to restore.
     * @param scheduled List of scheduled entries to restore.
     * @throws std::invalid_argument If any entry in @p scheduled has `time < a_now`.
     */
    void Restore(Ticks a_now, std::span<const SScheduled> scheduled)
    {
        timeline = {};
        current.clear();
        now = a_now;
        next_order = 0;

        for (const SScheduled& entry : scheduled) {
            if (entry.time < now) {
                throw std::invalid_argument("an actor cannot be scheduled to act before the time now");
            }

            Schedule(entry.id, entry.time - now);
        }
    }

    /**
     * @brief Advances time to the next scheduled event and returns the active actor.
     *
     * Pops the earliest active event from the timeline, updates @ref Now() to that
     * event's timestamp, and removes the actor from the active schedule. To act again,
     * the actor must be re-scheduled via @ref Schedule().
     *
     * Stale entries from rescheduled or removed actors are skipped automatically.
     *
     * @return An `std::optional` containing the next actor's @p Id, or `std::nullopt`
     *         if the timeline is empty.
     */
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

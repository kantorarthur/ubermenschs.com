#pragma once

#include <sw/redis++/redis++.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// A daily reminder: every day at "time" (in the user's time zone) the message is sent.
struct Reminder
{
    std::string id;
    std::string time;     // "07:30"
    std::string message;
    bool enabled = true;
    std::string timezone; // e.g. "Europe/Bucharest"
};

// A PATCH: only the fields that are set change.
struct ReminderChanges
{
    std::optional<std::string> time;
    std::optional<std::string> message;
    std::optional<bool> enabled;
    std::optional<std::string> timezone;
};

// The same limits the frontend shows (src/api/reminders.ts, REMINDER_LIMITS).
namespace limits
{
    inline constexpr std::size_t reminderMessage = 200;
    inline constexpr std::size_t remindersPerUser = 20;
}

// Reads and writes daily reminders in Redis; a user can have several:
//
//   next:reminder:id       counter for new ids (INCR)
//   reminder:{id}          hash: userId, time, message, enabled ("1"/"0"), timezone
//   user:{id}:reminders    set of the user's reminder ids
//   reminders:due          sorted set of the reminders that are on; the score is the next moment
//                          (seconds since 1970, UTC) the reminder goes off. ReminderScheduler reads it.
//
// A reminder of another user is reported as "does not exist" (NotFoundError), like presets.
inline const std::string DUE_REMINDERS_KEY = "reminders:due";

class ReminderStore
{
public:
    // Gives the current time; tests pass a fixed one.
    using Clock = std::function<std::chrono::sys_seconds()>;

    explicit ReminderStore(sw::redis::Redis& redis, Clock clock = systemClock);

    static std::chrono::sys_seconds systemClock();

    // Sorted by time of day.
    std::vector<Reminder> list(const std::string& userId);

    // A new reminder is on.
    Reminder create(const std::string& userId, const std::string& time, const std::string& message,
                    const std::string& timezone);
    Reminder update(const std::string& userId, const std::string& reminderId, const ReminderChanges& changes);
    void remove(const std::string& userId, const std::string& reminderId);

private:
    bool owns(const std::string& userId, const std::string& reminderId);
    Reminder load(const std::string& reminderId);
    // Puts the reminder into reminders:due at its next moment, or takes it out if it is off.
    void schedule(const Reminder& reminder);

    sw::redis::Redis& redis_;
    Clock clock_;
};

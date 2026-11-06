#pragma once

#include <sw/redis++/redis++.h>

#include <cstddef>
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
//
// A reminder of another user is reported as "does not exist" (NotFoundError), like presets.
class ReminderStore
{
public:
    explicit ReminderStore(sw::redis::Redis& redis);

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

    sw::redis::Redis& redis_;
};

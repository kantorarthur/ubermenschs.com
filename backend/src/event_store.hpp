#pragma once

#include <sw/redis++/redis++.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

// What the user fills in for an event. start and end are local times "YYYY-MM-DDTHH:MM";
// an all-day event starts at T00:00 and ends at T23:59 (the frontend sets that).
struct EventInput
{
    std::string title;
    std::string start;
    std::string end;
    bool allDay = false;
    std::string description;
    // When to send a notification, in minutes before the start (0 = at the start); any of NOTIFY_OPTIONS.
    // For an all-day event they count from 09:00 on its first day.
    std::vector<int> notify;
    std::string timezone; // the browser's time zone, e.g. "Europe/Bucharest"; needed when notify is not empty
};

struct CalendarEvent
{
    std::string id;
    std::string title;
    std::string start;
    std::string end;
    bool allDay = false;
    std::string description;
    std::vector<int> notify;
    std::string timezone;
};

// The notification choices: at the start, 10 and 30 minutes, 1 hour and 1 day before.
inline const std::vector<int> NOTIFY_OPTIONS = {0, 10, 30, 60, 1440};

// The time of day the notifications of an all-day event count from.
inline const std::string ALL_DAY_NOTIFY_TIME = "09:00";

// Sorted set of the event notifications still to send: member "{eventId}:{minutes}", score = the moment
// (UTC seconds since 1970). ReminderScheduler::sendDueEvents reads it.
inline const std::string DUE_EVENTS_KEY = "events:due";

// The same limits the frontend shows (src/api/events.ts, EVENT_LIMITS).
namespace limits
{
    inline constexpr std::size_t eventTitle = 100;
    inline constexpr std::size_t eventDescription = 500;
    inline constexpr std::size_t eventsPerUser = 2000;
    // The longest range one GET /api/events may ask for (a month view needs 42 days).
    inline constexpr int eventRangeDays = 400;
}

// Reads and writes calendar events in Redis:
//
//   next:event:id        counter for new ids (INCR)
//   event:{id}           hash: userId, title, start, end, allDay ("1"/"0"), description,
//                        notify ("0,10,1440"), timezone
//   user:{id}:events     sorted set of the user's event ids; the score is the start
//                        as a number (2026-11-06T14:30 → 202611061430), so events come out by start
//   events:due           the notifications still to send (see DUE_EVENTS_KEY)
//
// An event of another user is reported as "does not exist" (NotFoundError), like presets.
class EventStore
{
public:
    // Gives the current time; tests pass a fixed one.
    using Clock = std::function<std::chrono::sys_seconds()>;

    explicit EventStore(sw::redis::Redis& redis, Clock clock = systemClock);

    static std::chrono::sys_seconds systemClock();

    // Every event that touches at least one day from "from" to "to" (both "YYYY-MM-DD",
    // both included), sorted by start.
    std::vector<CalendarEvent> list(const std::string& userId, const std::string& from, const std::string& to);

    CalendarEvent create(const std::string& userId, const EventInput& input);
    // Replaces every field of the event.
    CalendarEvent update(const std::string& userId, const std::string& eventId, const EventInput& input);
    void remove(const std::string& userId, const std::string& eventId);

private:
    bool owns(const std::string& userId, const std::string& eventId);
    // The notifications of the event that are still in the future: member of events:due → moment.
    std::vector<std::pair<std::string, double>> dueNotifications(const std::string& eventId, const EventInput& event);

    sw::redis::Redis& redis_;
    Clock clock_;
};

// The moment a notification of the event goes off.
std::chrono::sys_seconds notificationTime(const EventInput& event, int minutesBefore);

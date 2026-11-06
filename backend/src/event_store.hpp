#pragma once

#include <sw/redis++/redis++.h>

#include <cstddef>
#include <string>
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
};

struct CalendarEvent
{
    std::string id;
    std::string title;
    std::string start;
    std::string end;
    bool allDay = false;
    std::string description;
};

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
//   event:{id}           hash: userId, title, start, end, allDay ("1"/"0"), description
//   user:{id}:events     sorted set of the user's event ids; the score is the start
//                        as a number (2026-11-06T14:30 → 202611061430), so events come out by start
//
// An event of another user is reported as "does not exist" (NotFoundError), like presets.
class EventStore
{
public:
    explicit EventStore(sw::redis::Redis& redis);

    // Every event that touches at least one day from "from" to "to" (both "YYYY-MM-DD",
    // both included), sorted by start.
    std::vector<CalendarEvent> list(const std::string& userId, const std::string& from, const std::string& to);

    CalendarEvent create(const std::string& userId, const EventInput& input);
    // Replaces every field of the event.
    CalendarEvent update(const std::string& userId, const std::string& eventId, const EventInput& input);
    void remove(const std::string& userId, const std::string& eventId);

private:
    bool owns(const std::string& userId, const std::string& eventId);

    sw::redis::Redis& redis_;
};

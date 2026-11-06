#include "event_store.hpp"

#include "datetime.hpp"
#include "errors.hpp"
#include "redis_client.hpp"
#include "text.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    std::string eventKey(const std::string& eventId)
    {
        return "event:" + eventId;
    }

    std::string userEventsKey(const std::string& userId)
    {
        return "user:" + userId + ":events";
    }

    // The checks the frontend also makes; returns the event with trimmed texts.
    EventInput cleanEvent(const EventInput& input)
    {
        EventInput cleaned = input;
        cleaned.title = requireText(input.title, limits::eventTitle, "The event needs a title.",
            "The title can have at most " + std::to_string(limits::eventTitle) + " characters.");

        cleaned.description = trim(input.description);
        if(utf8Length(cleaned.description) > limits::eventDescription)
            throw ValidationError("The description can have at most "
                                  + std::to_string(limits::eventDescription) + " characters.");

        if(!isLocalDateTime(input.start))
            throw ValidationError("Choose when the event starts.");
        if(!isLocalDateTime(input.end))
            throw ValidationError("Choose when the event ends.");
        // "YYYY-MM-DDTHH:MM" strings sort the same way as the moments they describe.
        if(input.end < input.start)
            throw ValidationError("The event cannot end before it starts.");

        // Notifications: only the offered choices, each once, in order.
        for(const int minutes : input.notify)
        {
            if(std::find(NOTIFY_OPTIONS.begin(), NOTIFY_OPTIONS.end(), minutes) == NOTIFY_OPTIONS.end())
                throw ValidationError("Unknown notification time: " + std::to_string(minutes) + " minutes before.");
        }
        std::sort(cleaned.notify.begin(), cleaned.notify.end());
        cleaned.notify.erase(std::unique(cleaned.notify.begin(), cleaned.notify.end()), cleaned.notify.end());

        if(!cleaned.timezone.empty() && !isTimeZone(cleaned.timezone))
            throw ValidationError("Unknown time zone: " + cleaned.timezone.substr(0, 64));
        if(!cleaned.notify.empty() && cleaned.timezone.empty())
            throw ValidationError("Notifications need the time zone of the event.");

        return cleaned;
    }

    // {0, 10, 1440} → "0,10,1440" and back.
    std::string joinNumbers(const std::vector<int>& numbers)
    {
        std::string text;
        for(const int n : numbers)
        {
            if(!text.empty())
                text += ',';
            text += std::to_string(n);
        }
        return text;
    }

    std::vector<int> splitNumbers(const std::string& text)
    {
        std::vector<int> numbers;
        std::stringstream stream(text);
        std::string item;
        while(std::getline(stream, item, ','))
        {
            if(!item.empty())
                numbers.push_back(std::stoi(item));
        }
        return numbers;
    }

    // Every member events:due can have for an event, to remove them all at once.
    std::vector<std::string> allDueMembers(const std::string& eventId)
    {
        std::vector<std::string> members;
        for(const int minutes : NOTIFY_OPTIONS)
            members.push_back(eventId + ":" + std::to_string(minutes));
        return members;
    }

    // The hash fields of an event, in the form HSET takes them.
    std::vector<std::pair<std::string, std::string>> fieldsOf(const std::string& userId, const EventInput& event)
    {
        return {
            {"userId", userId},
            {"title", event.title},
            {"start", event.start},
            {"end", event.end},
            {"allDay", event.allDay ? "1" : "0"},
            {"description", event.description},
            {"notify", joinNumbers(event.notify)},
            {"timezone", event.timezone},
        };
    }

    CalendarEvent toEvent(const std::string& eventId, const EventInput& input)
    {
        return CalendarEvent{eventId, input.title, input.start, input.end, input.allDay, input.description,
                             input.notify, input.timezone};
    }

    const std::string notFoundMessage = "This event does not exist.";
}

std::chrono::sys_seconds notificationTime(const EventInput& event, int minutesBefore)
{
    const std::string base = event.allDay ? event.start.substr(0, 10) + "T" + ALL_DAY_NOTIFY_TIME : event.start;
    return localToUtc(base, event.timezone) - std::chrono::minutes(minutesBefore);
}

EventStore::EventStore(sw::redis::Redis& redis, Clock clock)
    : redis_(redis), clock_(std::move(clock))
{
}

std::chrono::sys_seconds EventStore::systemClock()
{
    return std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
}

std::vector<std::pair<std::string, double>> EventStore::dueNotifications(const std::string& eventId,
                                                                         const EventInput& event)
{
    std::vector<std::pair<std::string, double>> due;
    const auto now = clock_();
    for(const int minutes : event.notify)
    {
        const auto moment = notificationTime(event, minutes);
        // Moments already past are not sent (e.g. "1 day before" for an event tomorrow morning at 08:00).
        if(moment > now)
        {
            due.emplace_back(eventId + ":" + std::to_string(minutes),
                             static_cast<double>(moment.time_since_epoch().count()));
        }
    }
    return due;
}

bool EventStore::owns(const std::string& userId, const std::string& eventId)
{
    if(!isId(eventId))
        return false;

    const auto owner = redis_.hget(eventKey(eventId), "userId");
    return owner && *owner == userId;
}

std::vector<CalendarEvent> EventStore::list(const std::string& userId, const std::string& from, const std::string& to)
{
    if(!isDate(from) || !isDate(to))
        throw ValidationError("\"from\" and \"to\" must be dates like 2026-11-06.");
    if(to < from)
        throw ValidationError("\"to\" cannot be before \"from\".");
    if(daysBetween(from, to) > limits::eventRangeDays)
        throw ValidationError("Ask for at most " + std::to_string(limits::eventRangeDays) + " days at once.");

    // Every event that starts on or before the last day, by start...
    std::vector<std::string> ids;
    redis_.zrangebyscore(userEventsKey(userId),
                         sw::redis::RightBoundedInterval<double>(
                             static_cast<double>(dateTimeNumber(to + "T23:59")), sw::redis::BoundType::LEFT_OPEN),
                         std::back_inserter(ids));
    if(ids.empty())
        return {};

    // ...read in one round trip (a pipeline) instead of one per event...
    auto pipeline = redis_.pipeline(false);
    for(const std::string& eventId : ids)
        pipeline.hgetall(eventKey(eventId));
    auto replies = pipeline.exec();

    // ...and kept if it has not ended before the first day.
    std::vector<CalendarEvent> events;
    for(std::size_t i = 0; i < ids.size(); ++i)
    {
        std::unordered_map<std::string, std::string> fields;
        replies.get(i, std::inserter(fields, fields.begin()));
        if(fields.empty() || fields["userId"] != userId)
            continue;
        if(fields["end"].substr(0, 10) < from)
            continue;

        events.push_back(CalendarEvent{ids[i], fields["title"], fields["start"], fields["end"],
                                       fields["allDay"] == "1", fields["description"],
                                       splitNumbers(fields["notify"]), fields["timezone"]});
    }
    return events;
}

CalendarEvent EventStore::create(const std::string& userId, const EventInput& input)
{
    const EventInput event = cleanEvent(input);
    if(static_cast<std::size_t>(redis_.zcard(userEventsKey(userId))) >= limits::eventsPerUser)
        throw ValidationError("You can have at most " + std::to_string(limits::eventsPerUser) + " events.");

    const std::string eventId = std::to_string(redis_.incr("next:event:id"));
    const auto fields = fieldsOf(userId, event);
    const auto due = dueNotifications(eventId, event);

    // The event, its place in the user's calendar and its notifications are written together or not at all.
    auto transaction = redis_.transaction();
    transaction.hset(eventKey(eventId), fields.begin(), fields.end())
               .zadd(userEventsKey(userId), eventId, static_cast<double>(dateTimeNumber(event.start)));
    if(!due.empty())
        transaction.zadd(DUE_EVENTS_KEY, due.begin(), due.end());
    transaction.exec();
    waitForReplica(transaction.redis());

    return toEvent(eventId, event);
}

CalendarEvent EventStore::update(const std::string& userId, const std::string& eventId, const EventInput& input)
{
    const EventInput event = cleanEvent(input);
    if(!owns(userId, eventId))
        throw NotFoundError(notFoundMessage);

    const auto fields = fieldsOf(userId, event);
    const auto due = dueNotifications(eventId, event);
    const auto oldDue = allDueMembers(eventId);

    // The start may have changed, so the score in the sorted set changes with it,
    // and the notifications are planned again from scratch.
    auto transaction = redis_.transaction();
    transaction.hset(eventKey(eventId), fields.begin(), fields.end())
               .zadd(userEventsKey(userId), eventId, static_cast<double>(dateTimeNumber(event.start)))
               .zrem(DUE_EVENTS_KEY, oldDue.begin(), oldDue.end());
    if(!due.empty())
        transaction.zadd(DUE_EVENTS_KEY, due.begin(), due.end());
    transaction.exec();
    waitForReplica(transaction.redis());

    return toEvent(eventId, event);
}

void EventStore::remove(const std::string& userId, const std::string& eventId)
{
    if(!owns(userId, eventId))
        throw NotFoundError(notFoundMessage);

    const auto oldDue = allDueMembers(eventId);
    auto transaction = redis_.transaction();
    transaction.del(eventKey(eventId))
               .zrem(userEventsKey(userId), eventId)
               .zrem(DUE_EVENTS_KEY, oldDue.begin(), oldDue.end());
    transaction.exec();
}

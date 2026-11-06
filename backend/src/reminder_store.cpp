#include "reminder_store.hpp"

#include "datetime.hpp"
#include "errors.hpp"
#include "redis_client.hpp"
#include "text.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    std::string reminderKey(const std::string& reminderId)
    {
        return "reminder:" + reminderId;
    }

    std::string userRemindersKey(const std::string& userId)
    {
        return "user:" + userId + ":reminders";
    }

    std::string checkTime(const std::string& time)
    {
        if(!isTimeOfDay(time))
            throw ValidationError("Choose a time.");
        return time;
    }

    std::string checkMessage(const std::string& message)
    {
        return requireText(message, limits::reminderMessage, "The reminder needs a message.",
            "The message can have at most " + std::to_string(limits::reminderMessage) + " characters.");
    }

    std::string checkTimeZone(const std::string& timezone)
    {
        if(!isTimeZone(timezone))
            throw ValidationError("Unknown time zone: " + timezone.substr(0, 64));
        return timezone;
    }

    const std::string notFoundMessage = "This reminder does not exist.";
}

ReminderStore::ReminderStore(sw::redis::Redis& redis)
    : redis_(redis)
{
}

bool ReminderStore::owns(const std::string& userId, const std::string& reminderId)
{
    if(!isId(reminderId))
        return false;

    const auto owner = redis_.hget(reminderKey(reminderId), "userId");
    return owner && *owner == userId;
}

Reminder ReminderStore::load(const std::string& reminderId)
{
    std::unordered_map<std::string, std::string> fields;
    redis_.hgetall(reminderKey(reminderId), std::inserter(fields, fields.begin()));
    return Reminder{reminderId, fields["time"], fields["message"], fields["enabled"] == "1", fields["timezone"]};
}

std::vector<Reminder> ReminderStore::list(const std::string& userId)
{
    std::vector<std::string> ids;
    redis_.smembers(userRemindersKey(userId), std::back_inserter(ids));

    std::vector<Reminder> reminders;
    for(const std::string& reminderId : ids)
    {
        if(owns(userId, reminderId))
            reminders.push_back(load(reminderId));
    }

    // By time of day; two reminders at the same time stay in the order they were made.
    std::sort(reminders.begin(), reminders.end(), [](const Reminder& a, const Reminder& b)
    {
        if(a.time != b.time)
            return a.time < b.time;
        return std::stoll(a.id) < std::stoll(b.id);
    });
    return reminders;
}

Reminder ReminderStore::create(const std::string& userId, const std::string& time, const std::string& message,
                               const std::string& timezone)
{
    Reminder reminder{"", checkTime(time), checkMessage(message), true, checkTimeZone(timezone)};
    if(static_cast<std::size_t>(redis_.scard(userRemindersKey(userId))) >= limits::remindersPerUser)
        throw ValidationError("You can have at most " + std::to_string(limits::remindersPerUser) + " reminders.");

    reminder.id = std::to_string(redis_.incr("next:reminder:id"));
    const std::vector<std::pair<std::string, std::string>> fields = {
        {"userId", userId},
        {"time", reminder.time},
        {"message", reminder.message},
        {"enabled", "1"},
        {"timezone", reminder.timezone},
    };

    auto transaction = redis_.transaction();
    transaction.hset(reminderKey(reminder.id), fields.begin(), fields.end())
               .sadd(userRemindersKey(userId), reminder.id);
    transaction.exec();
    waitForReplica(transaction.redis());

    return reminder;
}

Reminder ReminderStore::update(const std::string& userId, const std::string& reminderId,
                               const ReminderChanges& changes)
{
    // Check everything first, so a bad field changes nothing.
    std::vector<std::pair<std::string, std::string>> fields;
    if(changes.time)
        fields.emplace_back("time", checkTime(*changes.time));
    if(changes.message)
        fields.emplace_back("message", checkMessage(*changes.message));
    if(changes.enabled)
        fields.emplace_back("enabled", *changes.enabled ? "1" : "0");
    if(changes.timezone)
        fields.emplace_back("timezone", checkTimeZone(*changes.timezone));

    if(!owns(userId, reminderId))
        throw NotFoundError(notFoundMessage);

    if(!fields.empty())
        redis_.hset(reminderKey(reminderId), fields.begin(), fields.end());
    return load(reminderId);
}

void ReminderStore::remove(const std::string& userId, const std::string& reminderId)
{
    if(!owns(userId, reminderId))
        throw NotFoundError(notFoundMessage);

    auto transaction = redis_.transaction();
    transaction.del(reminderKey(reminderId))
               .srem(userRemindersKey(userId), reminderId);
    transaction.exec();
}

#include "reminder_scheduler.hpp"

#include "datetime.hpp"
#include "reminder_store.hpp"

#include <crow/json.h>
#include <crow/logging.h>

#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    // At most this many reminders per round; the rest follow 15 seconds later.
    constexpr long long BATCH = 200;

    std::string toJson(const Notification& notification)
    {
        crow::json::wvalue json;
        json["title"] = notification.title;
        json["body"] = notification.body;
        json["tag"] = notification.tag;
        json["url"] = notification.url;
        return json.dump();
    }
}

ReminderScheduler::ReminderScheduler(sw::redis::Redis& redis, PushStore& subscriptions, PushSender& sender)
    : redis_(redis), subscriptions_(subscriptions), sender_(sender)
{
}

int ReminderScheduler::sendDue(std::chrono::sys_seconds now)
{
    const double nowScore = static_cast<double>(now.time_since_epoch().count());

    std::vector<std::pair<std::string, double>> due;
    redis_.zrangebyscore(DUE_REMINDERS_KEY,
                         sw::redis::RightBoundedInterval<double>(nowScore, sw::redis::BoundType::LEFT_OPEN),
                         sw::redis::LimitOptions{0, BATCH}, std::back_inserter(due));

    int delivered = 0;
    for(const auto& [reminderId, score] : due)
    {
        std::unordered_map<std::string, std::string> fields;
        redis_.hgetall("reminder:" + reminderId, std::inserter(fields, fields.begin()));
        if(fields.empty() || fields["enabled"] != "1")
        {
            redis_.zrem(DUE_REMINDERS_KEY, reminderId);
            continue;
        }

        std::chrono::sys_seconds next;
        try
        {
            next = nextReminderTime(fields["time"], fields["timezone"], now);
        }
        catch(const std::runtime_error& error)
        {
            CROW_LOG_WARNING << "Reminder " << reminderId << " cannot be scheduled: " << error.what();
            redis_.zrem(DUE_REMINDERS_KEY, reminderId);
            continue;
        }

        // Another server got there first.
        if(!claim(reminderId, next))
            continue;

        const auto lateness = now - std::chrono::sys_seconds(std::chrono::seconds(static_cast<long long>(score)));
        if(lateness > maxLateness)
        {
            CROW_LOG_INFO << "Reminder " << reminderId << " skipped: " << lateness.count() << " s late";
            continue;
        }

        delivered += sendToUser(fields["userId"], Notification{"Reminder", fields["message"],
                                                               "reminder-" + reminderId, "/reminders"});
    }
    return delivered;
}

bool ReminderScheduler::claim(const std::string& reminderId, std::chrono::sys_seconds next)
{
    const long long changed = redis_.command<long long>("ZADD", DUE_REMINDERS_KEY, "XX", "GT", "CH",
                                                        std::to_string(next.time_since_epoch().count()),
                                                        reminderId);
    return changed == 1;
}

int ReminderScheduler::sendToUser(const std::string& userId, const Notification& notification)
{
    const std::string payload = toJson(notification);

    int delivered = 0;
    for(const PushSubscription& subscription : subscriptions_.list(userId))
    {
        const PushResult result = sender_.send(subscription, payload);
        if(result == PushResult::Delivered)
            ++delivered;
        else if(result == PushResult::Gone)
            subscriptions_.removeById(subscription.id);
    }
    return delivered;
}

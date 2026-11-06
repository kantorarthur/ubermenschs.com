#pragma once

#include "push_sender.hpp"
#include "push_store.hpp"

#include <sw/redis++/redis++.h>

#include <chrono>
#include <string>

// What a notification shows. The service worker (frontend/public/sw.js) reads these fields.
struct Notification
{
    std::string title;
    std::string body;
    std::string tag; // a new notification with the same tag replaces the old one on the screen
    std::string url; // the page that opens when the notification is clicked
};

// Sends the reminders that are due. main() calls sendDue every 15 seconds on its own thread.
//
// reminders:due (kept by ReminderStore) holds every reminder that is on, scored by the next moment it
// goes off. For each one that is due, the score is first moved to the following day with
// "ZADD XX GT CH": only the server that moves it sends the notification, so even with several
// servers running a reminder is sent once.
class ReminderScheduler
{
public:
    // A reminder more than this late (e.g. the server was down at 07:00) is skipped, not sent hours late.
    static constexpr std::chrono::minutes maxLateness{10};

    ReminderScheduler(sw::redis::Redis& redis, PushStore& subscriptions, PushSender& sender);

    // Sends every reminder due at `now`; returns how many notifications were delivered.
    int sendDue(std::chrono::sys_seconds now);

    // Sends a notification to every browser of the user; returns how many got it.
    // Subscriptions their push service no longer knows are forgotten.
    int sendToUser(const std::string& userId, const Notification& notification);

    // Moves a due reminder to its next moment, but only if nobody did that already: true means
    // this server may send it. (ZADD XX GT CH: only to a later score; the answer counts changes.)
    bool claim(const std::string& reminderId, std::chrono::sys_seconds next);

private:
    sw::redis::Redis& redis_;
    PushStore& subscriptions_;
    PushSender& sender_;
};

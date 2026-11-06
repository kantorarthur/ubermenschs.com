#include "event_store.hpp"
#include "push_store.hpp"
#include "redis_fixture.hpp"
#include "reminder_scheduler.hpp"
#include "reminder_store.hpp"
#include "webpush_crypto.hpp"

#include <crow/json.h>

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

namespace
{
    using namespace std::chrono;

    const std::string ANA = "1";
    const std::string BOB = "2";
    const std::string AUTH = base64UrlEncode("0123456789abcdef");

    // 6 November 2026, 05:00 UTC = 07:00 in Bucharest.
    const sys_seconds MORNING = sys_days{2026y / 11 / 6} + 5h;

    // Remembers what would have been sent, instead of sending it.
    class FakeSender : public PushSender
    {
    public:
        PushResult send(const PushSubscription& subscription, const std::string& payload) override
        {
            std::lock_guard<std::mutex> lock(mutex);
            sent.push_back({subscription.endpoint, payload});
            const auto planned = results.find(subscription.endpoint);
            return planned != results.end() ? planned->second : PushResult::Delivered;
        }

        struct Sent
        {
            std::string endpoint;
            std::string payload;
        };
        std::vector<Sent> sent;
        std::map<std::string, PushResult> results; // endpoint → answer; Delivered if not listed
        std::mutex mutex;
    };

    class ReminderSchedulerTest : public RedisTest
    {
    protected:
        void SetUp() override
        {
            RedisTest::SetUp();
            if(IsSkipped())
                return;
            subscriptions = std::make_unique<PushStore>(*redis, DEFAULT_PUSH_HOSTS);
            reminders = std::make_unique<ReminderStore>(*redis, [this]()
            {
                return now;
            });
            scheduler = std::make_unique<ReminderScheduler>(*redis, *subscriptions, sender);
        }

        void subscribe(const std::string& userId, const std::string& endpoint)
        {
            subscriptions->save(userId, endpoint, base64UrlEncode(generateP256KeyPair().publicKey), AUTH);
        }

        double dueScore(const std::string& reminderId)
        {
            const auto score = redis->zscore(DUE_REMINDERS_KEY, reminderId);
            return score ? *score : -1;
        }

        static double scoreOf(sys_seconds moment)
        {
            return static_cast<double>(moment.time_since_epoch().count());
        }

        sys_seconds now = MORNING - 1h;
        FakeSender sender;
        std::unique_ptr<PushStore> subscriptions;
        std::unique_ptr<ReminderStore> reminders;
        std::unique_ptr<ReminderScheduler> scheduler;
    };
}

TEST_F(ReminderSchedulerTest, ADueReminderGoesToEveryBrowserOfItsUser)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    subscribe(ANA, "https://updates.push.services.mozilla.com/wpush/v2/laptop");
    subscribe(BOB, "https://fcm.googleapis.com/fcm/send/bob");
    const Reminder r = reminders->create(ANA, "07:00", "Read 20 pages.", "Europe/Bucharest");

    // Not yet at 06:59:59.
    EXPECT_EQ(scheduler->sendDue(MORNING - 1s), 0);
    EXPECT_TRUE(sender.sent.empty());

    EXPECT_EQ(scheduler->sendDue(MORNING), 2);
    ASSERT_EQ(sender.sent.size(), 2u);
    for(const FakeSender::Sent& s : sender.sent)
    {
        EXPECT_EQ(s.endpoint.find("bob"), std::string::npos);
        const crow::json::rvalue payload = crow::json::load(s.payload);
        ASSERT_TRUE(payload);
        EXPECT_EQ(std::string(payload["title"].s()), "Reminder");
        EXPECT_EQ(std::string(payload["body"].s()), "Read 20 pages.");
        EXPECT_EQ(std::string(payload["tag"].s()), "reminder-" + r.id);
        EXPECT_EQ(std::string(payload["url"].s()), "/reminders");
    }

    // It now waits for tomorrow at 07:00.
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + days(1)));
}

TEST_F(ReminderSchedulerTest, AReminderIsSentOnlyOnce)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");

    EXPECT_EQ(scheduler->sendDue(MORNING + 5s), 1);
    // The next round, or a second server, finds nothing due.
    EXPECT_EQ(scheduler->sendDue(MORNING + 20s), 0);
    EXPECT_EQ(sender.sent.size(), 1u);
}

TEST_F(ReminderSchedulerTest, OnlyOneClaimWins)
{
    const Reminder r = reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");
    const sys_seconds tomorrow = MORNING + days(1);

    EXPECT_TRUE(scheduler->claim(r.id, tomorrow));
    EXPECT_FALSE(scheduler->claim(r.id, tomorrow));     // already moved there
    EXPECT_FALSE(scheduler->claim(r.id, MORNING));      // never moved back
    EXPECT_FALSE(scheduler->claim("999", tomorrow));    // XX: never adds a reminder
    EXPECT_FALSE(redis->zscore(DUE_REMINDERS_KEY, "999"));
    EXPECT_EQ(dueScore(r.id), scoreOf(tomorrow));
}

TEST_F(ReminderSchedulerTest, ManyServersAtOnceSendItOnce)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");

    // 8 "servers" run the same round at the same moment.
    std::vector<std::unique_ptr<ReminderScheduler>> servers;
    for(int i = 0; i < 8; ++i)
        servers.push_back(std::make_unique<ReminderScheduler>(*redis, *subscriptions, sender));

    std::vector<std::thread> threads;
    for(auto& server : servers)
    {
        threads.emplace_back([&server]()
        {
            server->sendDue(MORNING);
        });
    }
    for(std::thread& thread : threads)
        thread.join();

    EXPECT_EQ(sender.sent.size(), 1u);
}

TEST_F(ReminderSchedulerTest, ARoundThatIsLateStillSends)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");
    EXPECT_EQ(scheduler->sendDue(MORNING + 9min), 1);
}

TEST_F(ReminderSchedulerTest, AReminderMuchTooLateIsSkippedButStaysScheduled)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    const Reminder r = reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");

    // The server was down from 06:55 to 08:00.
    EXPECT_EQ(scheduler->sendDue(MORNING + 1h), 0);
    EXPECT_TRUE(sender.sent.empty());
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + days(1)));
}

TEST_F(ReminderSchedulerTest, ReminderThatIsOffOrDeletedIsDropped)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    const Reminder off = reminders->create(ANA, "07:00", "Off.", "Europe/Bucharest");
    const Reminder gone = reminders->create(ANA, "07:00", "Gone.", "Europe/Bucharest");

    // As if the change had happened between two rounds without reaching reminders:due.
    redis->hset("reminder:" + off.id, "enabled", "0");
    redis->del("reminder:" + gone.id);

    EXPECT_EQ(scheduler->sendDue(MORNING), 0);
    EXPECT_TRUE(sender.sent.empty());
    EXPECT_EQ(redis->zcard(DUE_REMINDERS_KEY), 0);
}

TEST_F(ReminderSchedulerTest, BrowsersThatUnsubscribedAreForgotten)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/old-phone");
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/flaky-laptop");
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/new-phone");
    sender.results["https://fcm.googleapis.com/fcm/send/old-phone"] = PushResult::Gone;
    sender.results["https://fcm.googleapis.com/fcm/send/flaky-laptop"] = PushResult::Failed;
    reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");

    EXPECT_EQ(scheduler->sendDue(MORNING), 1);

    // Gone: forgotten. Failed: kept, it may work next time.
    const std::vector<PushSubscription> left = subscriptions->list(ANA);
    ASSERT_EQ(left.size(), 2u);
    for(const PushSubscription& s : left)
        EXPECT_EQ(s.endpoint.find("old-phone"), std::string::npos);
}

TEST_F(ReminderSchedulerTest, NoBrowsersMeansNothingToSend)
{
    const Reminder r = reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");
    EXPECT_EQ(scheduler->sendDue(MORNING), 0);
    // Still moved to tomorrow, so it is not tried again every 15 seconds.
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + days(1)));
}

TEST_F(ReminderSchedulerTest, SendToUser)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    EXPECT_EQ(scheduler->sendToUser(ANA, Notification{"Title", "Body", "test", "/"}), 1);
    EXPECT_EQ(scheduler->sendToUser(BOB, Notification{"Title", "Body", "test", "/"}), 0);
}

// ReminderStore keeps reminders:due up to date.
TEST_F(ReminderSchedulerTest, TheStoreSchedulesEveryChange)
{
    const Reminder r = reminders->create(ANA, "07:00", "Read.", "Europe/Bucharest");
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING));

    ReminderChanges change;
    change.time = "07:30";
    reminders->update(ANA, r.id, change);
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + 30min));

    change = ReminderChanges{};
    change.timezone = "UTC";
    reminders->update(ANA, r.id, change);
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + 2h + 30min)); // 07:30 UTC

    change = ReminderChanges{};
    change.enabled = false;
    reminders->update(ANA, r.id, change);
    EXPECT_EQ(dueScore(r.id), -1);

    change.enabled = true;
    reminders->update(ANA, r.id, change);
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + 2h + 30min));

    // A message change keeps the time.
    change = ReminderChanges{};
    change.message = "Read 30 pages.";
    reminders->update(ANA, r.id, change);
    EXPECT_EQ(dueScore(r.id), scoreOf(MORNING + 2h + 30min));

    reminders->remove(ANA, r.id);
    EXPECT_EQ(dueScore(r.id), -1);
}

TEST_F(ReminderSchedulerTest, EventNotificationsAreSentOnceWithTheirText)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    subscribe(BOB, "https://fcm.googleapis.com/fcm/send/bob");
    EventStore events(*redis, [this]()
    {
        return now;
    });
    // 07:30 Bucharest = MORNING + 30 min; notifications at the start and 10 minutes before.
    EventInput exam{"Exam", "2026-11-06T07:30", "2026-11-06T09:00", false, "", {}, ""};
    exam.notify = {0, 10};
    exam.timezone = "Europe/Bucharest";
    const CalendarEvent e = events.create(ANA, exam);

    EXPECT_EQ(scheduler->sendDueEvents(MORNING + 19min), 0);
    EXPECT_EQ(scheduler->sendDueEvents(MORNING + 20min), 1);
    ASSERT_EQ(sender.sent.size(), 1u);
    crow::json::rvalue payload = crow::json::load(sender.sent[0].payload);
    EXPECT_EQ(std::string(payload["title"].s()), "Exam");
    EXPECT_EQ(std::string(payload["body"].s()), "Starts in 10 minutes, at 07:30");
    EXPECT_EQ(std::string(payload["tag"].s()), "event-" + e.id + "-10");
    EXPECT_EQ(std::string(payload["url"].s()), "/calendar");
    EXPECT_EQ(sender.sent[0].endpoint.find("bob"), std::string::npos);

    // Not again in the next round.
    EXPECT_EQ(scheduler->sendDueEvents(MORNING + 21min), 0);

    EXPECT_EQ(scheduler->sendDueEvents(MORNING + 30min), 1);
    payload = crow::json::load(sender.sent[1].payload);
    EXPECT_EQ(std::string(payload["body"].s()), "Starts now (07:30)");
    EXPECT_EQ(redis->zcard(DUE_EVENTS_KEY), 0);
}

TEST_F(ReminderSchedulerTest, LateOrDeletedEventNotificationsAreDropped)
{
    subscribe(ANA, "https://fcm.googleapis.com/fcm/send/phone");
    EventStore events(*redis, [this]()
    {
        return now;
    });
    EventInput late{"Late", "2026-11-06T07:30", "2026-11-06T08:00", false, "", {}, ""};
    late.notify = {0};
    late.timezone = "Europe/Bucharest";
    events.create(ANA, late);

    EventInput gone = late;
    gone.title = "Gone";
    const CalendarEvent g = events.create(ANA, gone);
    redis->del("event:" + g.id); // as if deleted without reaching events:due

    // The server was down from 07:25 to 08:00.
    EXPECT_EQ(scheduler->sendDueEvents(MORNING + 1h), 0);
    EXPECT_TRUE(sender.sent.empty());
    EXPECT_EQ(redis->zcard(DUE_EVENTS_KEY), 0);
}

TEST(EventNotificationText, AllDayAndOneDayBefore)
{
    // Checked through the scheduler's public result above for timed events; here the all-day wording.
    EventInput holiday{"Holiday", "2026-11-08T00:00", "2026-11-08T23:59", true, "", {}, ""};
    holiday.timezone = "Europe/Bucharest";
    EXPECT_EQ(notificationTime(holiday, 1440), notificationTime(holiday, 0) - std::chrono::hours(24));
}

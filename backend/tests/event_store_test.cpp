#include "errors.hpp"
#include "event_store.hpp"
#include "redis_fixture.hpp"

#include <chrono>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    const std::string ANA = "1";
    const std::string BOB = "2";

    EventInput event(const std::string& title, const std::string& start, const std::string& end)
    {
        return EventInput{title, start, end, false, "", {}, ""};
    }

    std::vector<std::string> titles(const std::vector<CalendarEvent>& events)
    {
        std::vector<std::string> result;
        for(const CalendarEvent& e : events)
            result.push_back(e.title);
        return result;
    }

    class EventStoreTest : public RedisTest
    {
    };
}

TEST_F(EventStoreTest, CreateAndListSortedByStart)
{
    EventStore store(*redis);
    store.create(ANA, event("Lunch", "2026-11-06T13:00", "2026-11-06T14:00"));
    const CalendarEvent run = store.create(ANA, EventInput{"  Run  ", "2026-11-06T07:00", "2026-11-06T08:00",
                                                           false, "  5 km  ", {}, ""});
    store.create(ANA, EventInput{"Holiday", "2026-11-05T00:00", "2026-11-05T23:59", true, "", {}, ""});

    EXPECT_EQ(run.title, "Run");
    EXPECT_EQ(run.description, "5 km");

    const std::vector<CalendarEvent> events = store.list(ANA, "2026-11-01", "2026-11-30");
    EXPECT_EQ(titles(events), (std::vector<std::string>{"Holiday", "Run", "Lunch"}));
    EXPECT_TRUE(events[0].allDay);
    EXPECT_FALSE(events[1].allDay);
    EXPECT_EQ(events[1].start, "2026-11-06T07:00");
    EXPECT_EQ(events[1].end, "2026-11-06T08:00");
}

TEST_F(EventStoreTest, ListReturnsOnlyEventsThatTouchTheRange)
{
    EventStore store(*redis);
    store.create(ANA, event("Before", "2026-10-30T10:00", "2026-10-31T23:00"));
    store.create(ANA, event("Starts before, ends inside", "2026-10-31T22:00", "2026-11-01T02:00"));
    store.create(ANA, event("Inside", "2026-11-15T10:00", "2026-11-15T11:00"));
    store.create(ANA, event("On the last day", "2026-11-30T23:00", "2026-12-02T10:00"));
    store.create(ANA, event("Covers the whole month", "2026-10-01T00:00", "2026-12-31T23:59"));
    store.create(ANA, event("After", "2026-12-01T00:00", "2026-12-01T01:00"));

    EXPECT_EQ(titles(store.list(ANA, "2026-11-01", "2026-11-30")),
              (std::vector<std::string>{"Covers the whole month", "Starts before, ends inside", "Inside",
                                        "On the last day"}));
    EXPECT_TRUE(store.list(ANA, "2027-01-01", "2027-01-31").empty());
}

TEST_F(EventStoreTest, EventRules)
{
    EventStore store(*redis);
    EXPECT_THROW(store.create(ANA, event("  ", "2026-11-06T10:00", "2026-11-06T11:00")), ValidationError);
    EXPECT_THROW(store.create(ANA, event(std::string(101, 'x'), "2026-11-06T10:00", "2026-11-06T11:00")),
                 ValidationError);
    EXPECT_THROW(store.create(ANA, event("Late", "2026-11-06T10:00", "2026-11-06T09:59")), ValidationError);
    EXPECT_THROW(store.create(ANA, event("Bad date", "2026-02-30T10:00", "2026-03-01T10:00")), ValidationError);
    EXPECT_THROW(store.create(ANA, event("Bad format", "2026-11-06 10:00", "2026-11-06T11:00")), ValidationError);
    EXPECT_THROW(store.create(ANA, EventInput{"Long", "2026-11-06T10:00", "2026-11-06T11:00", false,
                                              std::string(501, 'x'), {}, ""}), ValidationError);

    EXPECT_NO_THROW(store.create(ANA, event(std::string(100, 'x'), "2026-11-06T10:00", "2026-11-06T10:00")));
    // Nothing was saved by the refused ones.
    EXPECT_EQ(store.list(ANA, "2026-01-01", "2026-12-31").size(), 1u);
}

TEST_F(EventStoreTest, RangeRules)
{
    EventStore store(*redis);
    EXPECT_THROW(store.list(ANA, "", "2026-11-30"), ValidationError);
    EXPECT_THROW(store.list(ANA, "2026-11-01", "tomorrow"), ValidationError);
    EXPECT_THROW(store.list(ANA, "2026-11-30", "2026-11-01"), ValidationError);
    EXPECT_THROW(store.list(ANA, "2026-01-01", "2027-12-31"), ValidationError);
    EXPECT_NO_THROW(store.list(ANA, "2026-11-06", "2026-11-06"));
}

TEST_F(EventStoreTest, UpdateReplacesTheEventAndItsPlace)
{
    EventStore store(*redis);
    const CalendarEvent first = store.create(ANA, event("First", "2026-11-06T09:00", "2026-11-06T10:00"));
    store.create(ANA, event("Second", "2026-11-06T12:00", "2026-11-06T13:00"));

    const CalendarEvent moved = store.update(ANA, first.id,
        EventInput{"Moved", "2026-11-06T15:00", "2026-11-06T16:00", false, "later", {}, ""});
    EXPECT_EQ(moved.id, first.id);
    EXPECT_EQ(moved.title, "Moved");

    // The new start puts it after "Second".
    EXPECT_EQ(titles(store.list(ANA, "2026-11-06", "2026-11-06")), (std::vector<std::string>{"Second", "Moved"}));
    // A bad update changes nothing.
    EXPECT_THROW(store.update(ANA, first.id, event("", "2026-11-06T15:00", "2026-11-06T16:00")), ValidationError);
    EXPECT_EQ(titles(store.list(ANA, "2026-11-06", "2026-11-06")), (std::vector<std::string>{"Second", "Moved"}));
}

TEST_F(EventStoreTest, DeleteRemovesEveryKeyOfTheEvent)
{
    EventStore store(*redis);
    const CalendarEvent e = store.create(ANA, event("Gone soon", "2026-11-06T09:00", "2026-11-06T10:00"));

    store.remove(ANA, e.id);

    EXPECT_TRUE(store.list(ANA, "2026-11-06", "2026-11-06").empty());
    std::vector<std::string> keys;
    redis->keys("*", std::back_inserter(keys));
    for(const std::string& key : keys)
        EXPECT_TRUE(key.rfind("next:", 0) == 0) << "left behind: " << key;
}

TEST_F(EventStoreTest, AnotherUserCannotSeeOrChangeTheEvent)
{
    EventStore store(*redis);
    const CalendarEvent e = store.create(ANA, event("Ana's", "2026-11-06T09:00", "2026-11-06T10:00"));

    EXPECT_TRUE(store.list(BOB, "2026-11-01", "2026-11-30").empty());
    EXPECT_THROW(store.update(BOB, e.id, event("Bob's now", "2026-11-06T09:00", "2026-11-06T10:00")),
                 NotFoundError);
    EXPECT_THROW(store.remove(BOB, e.id), NotFoundError);

    const std::vector<CalendarEvent> anas = store.list(ANA, "2026-11-06", "2026-11-06");
    ASSERT_EQ(anas.size(), 1u);
    EXPECT_EQ(anas[0].title, "Ana's");
}

TEST_F(EventStoreTest, BadIdsAreNotFound)
{
    EventStore store(*redis);
    EXPECT_THROW(store.remove(ANA, "abc"), NotFoundError);
    EXPECT_THROW(store.remove(ANA, "999"), NotFoundError);
    EXPECT_THROW(store.update(ANA, "../1", event("x", "2026-11-06T09:00", "2026-11-06T10:00")), NotFoundError);
}

TEST_F(EventStoreTest, AUserHasAtMostTheLimitOfEvents)
{
    EventStore store(*redis);
    // Fill the sorted set directly; creating 2000 events one by one would only slow the test down.
    for(std::size_t i = 0; i < limits::eventsPerUser; ++i)
        redis->zadd("user:" + ANA + ":events", "fake" + std::to_string(i), 0);

    EXPECT_THROW(store.create(ANA, event("One too many", "2026-11-06T09:00", "2026-11-06T10:00")), ValidationError);
    EXPECT_NO_THROW(store.create(BOB, event("Bob's first", "2026-11-06T09:00", "2026-11-06T10:00")));
}

namespace
{
    using namespace std::chrono;

    // 6 November 2026, 10:00 in Bucharest (08:00 UTC).
    const sys_seconds TEN_IN_BUCHAREST = sys_days{2026y / 11 / 6} + 8h;

    EventInput withNotifications(const std::string& title, const std::string& start, std::vector<int> notify)
    {
        EventInput input{title, start, start, false, "", {}, ""};
        input.notify = std::move(notify);
        input.timezone = "Europe/Bucharest";
        return input;
    }

    double scoreOf(sys_seconds moment)
    {
        return static_cast<double>(moment.time_since_epoch().count());
    }

    class EventNotificationsTest : public RedisTest
    {
    protected:
        double dueScore(const std::string& member)
        {
            const auto score = redis->zscore(DUE_EVENTS_KEY, member);
            return score ? *score : -1;
        }
    };
}

TEST_F(EventNotificationsTest, TheChoicesAreSavedAndPlanned)
{
    EventStore store(*redis, []()
    {
        return TEN_IN_BUCHAREST;
    });
    // Tomorrow at 14:30 Bucharest time = 12:30 UTC.
    const CalendarEvent e = store.create(ANA, withNotifications("Exam", "2026-11-07T14:30", {1440, 10, 10, 0}));

    EXPECT_EQ(e.notify, (std::vector<int>{0, 10, 1440})); // sorted, each once
    EXPECT_EQ(e.timezone, "Europe/Bucharest");
    const sys_seconds start = sys_days{2026y / 11 / 7} + 12h + 30min;
    EXPECT_EQ(dueScore(e.id + ":0"), scoreOf(start));
    EXPECT_EQ(dueScore(e.id + ":10"), scoreOf(start - 10min));
    EXPECT_EQ(dueScore(e.id + ":1440"), scoreOf(start - 24h));

    const CalendarEvent listed = store.list(ANA, "2026-11-07", "2026-11-07")[0];
    EXPECT_EQ(listed.notify, (std::vector<int>{0, 10, 1440}));
    EXPECT_EQ(listed.timezone, "Europe/Bucharest");
}

TEST_F(EventNotificationsTest, MomentsAlreadyPastAreNotPlanned)
{
    EventStore store(*redis, []()
    {
        return TEN_IN_BUCHAREST;
    });
    // Today at 10:05: "at the start" is still ahead, "10 minutes before" (09:55) and "1 day before" are past.
    const CalendarEvent e = store.create(ANA, withNotifications("Soon", "2026-11-06T10:05", {0, 10, 1440}));
    EXPECT_GT(dueScore(e.id + ":0"), 0);
    EXPECT_EQ(dueScore(e.id + ":10"), -1);
    EXPECT_EQ(dueScore(e.id + ":1440"), -1);
}

TEST_F(EventNotificationsTest, AllDayEventsCountFromNineInTheMorning)
{
    EventStore store(*redis, []()
    {
        return TEN_IN_BUCHAREST;
    });
    EventInput holiday{"Holiday", "2026-11-08T00:00", "2026-11-08T23:59", true, "", {}, ""};
    holiday.notify = {0, 1440};
    holiday.timezone = "Europe/Bucharest";
    const CalendarEvent e = store.create(ANA, holiday);

    const sys_seconds nine = sys_days{2026y / 11 / 8} + 7h; // 09:00 Bucharest
    EXPECT_EQ(dueScore(e.id + ":0"), scoreOf(nine));
    EXPECT_EQ(dueScore(e.id + ":1440"), scoreOf(nine - 24h));
}

TEST_F(EventNotificationsTest, UpdateAndDeletePlanAgain)
{
    EventStore store(*redis, []()
    {
        return TEN_IN_BUCHAREST;
    });
    const CalendarEvent e = store.create(ANA, withNotifications("Exam", "2026-11-07T14:30", {10, 1440}));

    // Moved one hour later, and only "at the start".
    store.update(ANA, e.id, withNotifications("Exam", "2026-11-07T15:30", {0}));
    EXPECT_EQ(dueScore(e.id + ":10"), -1);
    EXPECT_EQ(dueScore(e.id + ":1440"), -1);
    EXPECT_EQ(dueScore(e.id + ":0"), scoreOf(sys_days{2026y / 11 / 7} + 13h + 30min));

    // No notifications any more.
    store.update(ANA, e.id, EventInput{"Exam", "2026-11-07T15:30", "2026-11-07T15:30", false, "", {}, ""});
    EXPECT_EQ(redis->zcard(DUE_EVENTS_KEY), 0);

    store.update(ANA, e.id, withNotifications("Exam", "2026-11-07T15:30", {0, 60}));
    store.remove(ANA, e.id);
    EXPECT_EQ(redis->zcard(DUE_EVENTS_KEY), 0);
}

TEST_F(EventNotificationsTest, BadChoicesAreRefused)
{
    EventStore store(*redis);
    EXPECT_THROW(store.create(ANA, withNotifications("x", "2026-11-07T14:30", {5})), ValidationError);
    EXPECT_THROW(store.create(ANA, withNotifications("x", "2026-11-07T14:30", {-10})), ValidationError);

    EventInput noZone = withNotifications("x", "2026-11-07T14:30", {10});
    noZone.timezone = "";
    EXPECT_THROW(store.create(ANA, noZone), ValidationError);
    noZone.timezone = "Mars/Base";
    EXPECT_THROW(store.create(ANA, noZone), ValidationError);

    EXPECT_TRUE(store.list(ANA, "2026-11-01", "2026-11-30").empty());
    EXPECT_EQ(redis->zcard(DUE_EVENTS_KEY), 0);
}

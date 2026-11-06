#include "errors.hpp"
#include "redis_fixture.hpp"
#include "reminder_store.hpp"

#include <iterator>
#include <string>
#include <vector>

namespace
{
    const std::string ANA = "1";
    const std::string BOB = "2";
    const std::string BUCHAREST = "Europe/Bucharest";

    class ReminderStoreTest : public RedisTest
    {
    };
}

TEST_F(ReminderStoreTest, AUserCanHaveSeveralRemindersSortedByTime)
{
    ReminderStore store(*redis);
    store.create(ANA, "21:00", "Write down one thing you learned.", BUCHAREST);
    const Reminder morning = store.create(ANA, "07:00", "  Read 20 pages.  ", BUCHAREST);
    store.create(ANA, "07:00", "Drink water.", BUCHAREST);

    EXPECT_EQ(morning.message, "Read 20 pages.");
    EXPECT_TRUE(morning.enabled);
    EXPECT_EQ(morning.timezone, BUCHAREST);

    const std::vector<Reminder> reminders = store.list(ANA);
    ASSERT_EQ(reminders.size(), 3u);
    EXPECT_EQ(reminders[0].message, "Read 20 pages."); // same time: the older one first
    EXPECT_EQ(reminders[1].message, "Drink water.");
    EXPECT_EQ(reminders[2].time, "21:00");
}

TEST_F(ReminderStoreTest, ReminderRules)
{
    ReminderStore store(*redis);
    EXPECT_THROW(store.create(ANA, "24:00", "Late", BUCHAREST), ValidationError);
    EXPECT_THROW(store.create(ANA, "", "No time", BUCHAREST), ValidationError);
    EXPECT_THROW(store.create(ANA, "07:00", "   ", BUCHAREST), ValidationError);
    EXPECT_THROW(store.create(ANA, "07:00", std::string(201, 'x'), BUCHAREST), ValidationError);
    EXPECT_THROW(store.create(ANA, "07:00", "Wake up", "Europe/Atlantis"), ValidationError);
    EXPECT_NO_THROW(store.create(ANA, "07:00", std::string(200, 'x'), "UTC"));
    EXPECT_EQ(store.list(ANA).size(), 1u);
}

TEST_F(ReminderStoreTest, UpdateChangesOnlyTheGivenFields)
{
    ReminderStore store(*redis);
    const Reminder r = store.create(ANA, "07:00", "Read", BUCHAREST);

    ReminderChanges off;
    off.enabled = false;
    Reminder updated = store.update(ANA, r.id, off);
    EXPECT_FALSE(updated.enabled);
    EXPECT_EQ(updated.time, "07:00");
    EXPECT_EQ(updated.message, "Read");

    ReminderChanges moved;
    moved.time = "06:30";
    moved.message = " Read 30 pages ";
    moved.timezone = "America/New_York";
    updated = store.update(ANA, r.id, moved);
    EXPECT_EQ(updated.time, "06:30");
    EXPECT_EQ(updated.message, "Read 30 pages");
    EXPECT_EQ(updated.timezone, "America/New_York");
    EXPECT_FALSE(updated.enabled); // still off

    EXPECT_EQ(store.list(ANA)[0].time, "06:30");
}

TEST_F(ReminderStoreTest, ABadUpdateChangesNothing)
{
    ReminderStore store(*redis);
    const Reminder r = store.create(ANA, "07:00", "Read", BUCHAREST);

    ReminderChanges changes;
    changes.message = "New message";
    changes.time = "25:00";
    EXPECT_THROW(store.update(ANA, r.id, changes), ValidationError);

    EXPECT_EQ(store.list(ANA)[0].message, "Read");
    EXPECT_EQ(store.list(ANA)[0].time, "07:00");
}

TEST_F(ReminderStoreTest, DeleteRemovesEveryKeyOfTheReminder)
{
    ReminderStore store(*redis);
    const Reminder r = store.create(ANA, "07:00", "Read", BUCHAREST);

    store.remove(ANA, r.id);

    EXPECT_TRUE(store.list(ANA).empty());
    std::vector<std::string> keys;
    redis->keys("*", std::back_inserter(keys));
    for(const std::string& key : keys)
        EXPECT_TRUE(key.rfind("next:", 0) == 0) << "left behind: " << key;
}

TEST_F(ReminderStoreTest, AnotherUserCannotSeeOrChangeTheReminder)
{
    ReminderStore store(*redis);
    const Reminder r = store.create(ANA, "07:00", "Read", BUCHAREST);

    ReminderChanges off;
    off.enabled = false;
    EXPECT_TRUE(store.list(BOB).empty());
    EXPECT_THROW(store.update(BOB, r.id, off), NotFoundError);
    EXPECT_THROW(store.remove(BOB, r.id), NotFoundError);
    EXPECT_THROW(store.remove(ANA, "abc"), NotFoundError);

    const std::vector<Reminder> anas = store.list(ANA);
    ASSERT_EQ(anas.size(), 1u);
    EXPECT_TRUE(anas[0].enabled);
}

TEST_F(ReminderStoreTest, AUserHasAtMostTheLimitOfReminders)
{
    ReminderStore store(*redis);
    for(std::size_t i = 0; i < limits::remindersPerUser; ++i)
        store.create(ANA, "07:00", "Reminder " + std::to_string(i), BUCHAREST);

    EXPECT_THROW(store.create(ANA, "08:00", "One too many", BUCHAREST), ValidationError);
    EXPECT_EQ(store.list(ANA).size(), limits::remindersPerUser);

    // After deleting one, there is room again.
    store.remove(ANA, store.list(ANA)[0].id);
    EXPECT_NO_THROW(store.create(ANA, "08:00", "Now it fits", BUCHAREST));
}

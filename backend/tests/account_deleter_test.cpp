#include "account_deleter.hpp"
#include "crypto.hpp"
#include "errors.hpp"
#include "event_store.hpp"
#include "preset_store.hpp"
#include "push_store.hpp"
#include "redis_fixture.hpp"
#include "reminder_store.hpp"
#include "study_store.hpp"
#include "user_store.hpp"
#include "webpush_crypto.hpp"

#include <chrono>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace
{
    const std::string PASSWORD = "a long enough password";
    const std::string BUCHAREST = "Europe/Bucharest";
    const std::string AUTH = base64UrlEncode("0123456789abcdef");

    std::chrono::sys_seconds fixedNow()
    {
        using namespace std::chrono;
        return sys_days{year{2026} / 11 / 20};
    }

    class AccountDeleterTest : public RedisTest
    {
    protected:
        void SetUp() override
        {
            RedisTest::SetUp();
            if(IsSkipped())
                return;
            users = std::make_unique<UserStore>(*redis);
            sessions = std::make_unique<SessionStore>(*redis);
            presets = std::make_unique<PresetStore>(*redis);
            events = std::make_unique<EventStore>(*redis, fixedNow);
            reminders = std::make_unique<ReminderStore>(*redis, fixedNow);
            push = std::make_unique<PushStore>(*redis, DEFAULT_PUSH_HOSTS);
            study = std::make_unique<StudyStore>(*redis);
            deleter = std::make_unique<AccountDeleter>(*users, *sessions, *presets, *events, *reminders, *push,
                                                       *study);
        }

        // An account with some of everything: two logins, a quote set in use, an event with
        // notifications, a reminder, a browser for notifications and a book read.
        User fullAccount(const std::string& email, const std::string& endpoint)
        {
            const User user = users->registerUser(email, "Someone", PASSWORD);
            sessions->create(user.id);
            sessions->create(user.id);

            const Preset preset = presets->create(user.id, "Mine");
            presets->addQuote(user.id, preset.id, "Know thyself.", "Socrates");
            presets->addQuote(user.id, preset.id, "Become who you are.", "Nietzsche");
            presets->setActivePreset(user.id, preset.id);

            events->create(user.id, EventInput{"Exam", "2026-12-10T09:00", "2026-12-10T11:00", false, "",
                                               {0, 60, 1440}, BUCHAREST});
            events->create(user.id, EventInput{"Holiday", "2026-12-24T00:00", "2026-12-24T23:59", true, "", {0},
                                               BUCHAREST});

            reminders->create(user.id, "07:00", "Read 20 pages.", BUCHAREST);
            push->save(user.id, endpoint, base64UrlEncode(generateP256KeyPair().publicKey), AUTH);
            study->setRead(user.id, "meditations", true);
            return user;
        }

        std::set<std::string> allKeys()
        {
            std::vector<std::string> keys;
            redis->keys("*", std::back_inserter(keys));
            return {keys.begin(), keys.end()};
        }

        std::set<std::string> members(const std::string& sortedSet)
        {
            std::vector<std::string> result;
            redis->zrange(sortedSet, 0, -1, std::back_inserter(result));
            return {result.begin(), result.end()};
        }

        std::unique_ptr<UserStore> users;
        std::unique_ptr<SessionStore> sessions;
        std::unique_ptr<PresetStore> presets;
        std::unique_ptr<EventStore> events;
        std::unique_ptr<ReminderStore> reminders;
        std::unique_ptr<PushStore> push;
        std::unique_ptr<StudyStore> study;
        std::unique_ptr<AccountDeleter> deleter;
    };
}

TEST_F(AccountDeleterTest, DeletesEverythingOfTheUserAndNothingElse)
{
    const User bob = fullAccount("bob@example.com", "https://fcm.googleapis.com/fcm/send/bob");
    const std::string bobSession = sessions->create(bob.id);
    // Ana's ids are created after Bob's, so the id counters already exist before her account.
    const std::set<std::string> keysBefore = allKeys();
    const std::set<std::string> eventsDueBefore = members(DUE_EVENTS_KEY);
    const std::set<std::string> remindersDueBefore = members(DUE_REMINDERS_KEY);

    const User ana = fullAccount("ana@example.com", "https://fcm.googleapis.com/fcm/send/ana");
    ASSERT_GT(allKeys().size(), keysBefore.size());
    ASSERT_GT(members(DUE_EVENTS_KEY).size(), eventsDueBefore.size());

    deleter->deleteAccount(ana.id);

    EXPECT_EQ(allKeys(), keysBefore);
    EXPECT_EQ(members(DUE_EVENTS_KEY), eventsDueBefore);
    EXPECT_EQ(members(DUE_REMINDERS_KEY), remindersDueBefore);
    EXPECT_FALSE(users->findById(ana.id));

    // Bob does not notice anything.
    EXPECT_EQ(sessions->userIdFor(bobSession).value(), bob.id);
    EXPECT_EQ(presets->list(bob.id).size(), 1u);
    EXPECT_EQ(presets->list(bob.id)[0].quotes.size(), 2u);
    EXPECT_EQ(reminders->list(bob.id).size(), 1u);
    EXPECT_EQ(push->list(bob.id).size(), 1u);
    EXPECT_EQ(study->readBooks(bob.id), std::vector<std::string>{"meditations"});
}

TEST_F(AccountDeleterTest, EveryLoginEnds)
{
    const User ana = users->registerUser("ana@example.com", "Ana", PASSWORD);
    const std::string phone = sessions->create(ana.id);
    const std::string laptop = sessions->create(ana.id);

    deleter->deleteAccount(ana.id);

    EXPECT_FALSE(sessions->userIdFor(phone));
    EXPECT_FALSE(sessions->userIdFor(laptop));
    EXPECT_FALSE(redis->exists("user:" + ana.id + ":sessions"));
}

TEST_F(AccountDeleterTest, TheEmailCanBeUsedAgain)
{
    const User first = users->registerUser("ana@example.com", "Ana", PASSWORD);
    deleter->deleteAccount(first.id);

    EXPECT_FALSE(users->authenticate("ana@example.com", PASSWORD));
    const User second = users->registerUser("ana@example.com", "Ana again", "another long password");
    EXPECT_NE(second.id, first.id);
    EXPECT_TRUE(users->authenticate("ana@example.com", "another long password"));
}

TEST_F(AccountDeleterTest, ABrowserThatMovedToAnotherAccountStaysThere)
{
    const std::string sharedBrowser = "https://fcm.googleapis.com/fcm/send/shared";
    const User ana = users->registerUser("ana@example.com", "Ana", PASSWORD);
    const User bob = users->registerUser("bob@example.com", "Bob", PASSWORD);
    push->save(ana.id, sharedBrowser, base64UrlEncode(generateP256KeyPair().publicKey), AUTH);
    // Bob logs in on the same browser and turns notifications on: the browser moves to him.
    push->save(bob.id, sharedBrowser, base64UrlEncode(generateP256KeyPair().publicKey), AUTH);
    // Even if Ana's list still had it (e.g. data from an older version), it is not deleted.
    redis->sadd("user:" + ana.id + ":push", fingerprint(sharedBrowser));

    deleter->deleteAccount(ana.id);

    ASSERT_EQ(push->list(bob.id).size(), 1u);
    EXPECT_EQ(push->list(bob.id)[0].endpoint, sharedBrowser);
}

TEST_F(AccountDeleterTest, AnEmptyAccountCanBeDeleted)
{
    const User ana = users->registerUser("ana@example.com", "Ana", PASSWORD);
    EXPECT_NO_THROW(deleter->deleteAccount(ana.id));
    // Only the id counter is left.
    EXPECT_EQ(allKeys(), std::set<std::string>{"next:user:id"});
}

TEST_F(AccountDeleterTest, PasswordMatches)
{
    const User ana = users->registerUser("ana@example.com", "Ana", PASSWORD);
    EXPECT_TRUE(users->passwordMatches(ana.id, PASSWORD));
    EXPECT_FALSE(users->passwordMatches(ana.id, "a wrong password"));
    EXPECT_FALSE(users->passwordMatches(ana.id, ""));
    EXPECT_FALSE(users->passwordMatches("999", PASSWORD));
}

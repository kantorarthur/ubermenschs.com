#include "redis_fixture.hpp"
#include "user_store.hpp"

#include <iterator>
#include <string>
#include <vector>

namespace
{
    class SessionStoreTest : public RedisTest
    {
    };
}

TEST_F(SessionStoreTest, ATokenLeadsBackToItsUser)
{
    SessionStore sessions(*redis);
    const std::string token = sessions.create("7");

    EXPECT_EQ(token.size(), 64u);
    EXPECT_EQ(sessions.userIdFor(token).value(), "7");
}

TEST_F(SessionStoreTest, RedisKeepsOnlyAFingerprintOfTheToken)
{
    SessionStore sessions(*redis);
    const std::string token = sessions.create("7");

    std::vector<std::string> keys;
    redis->keys("*", std::back_inserter(keys));
    EXPECT_EQ(keys.size(), 2u); // the session and the user's list of sessions
    EXPECT_TRUE(redis->exists("session:" + fingerprint(token)));
    EXPECT_TRUE(redis->sismember("user:7:sessions", fingerprint(token)));
    // The token itself is nowhere: not in a key, not in the list.
    for(const std::string& key : keys)
        EXPECT_EQ(key.find(token), std::string::npos);
    EXPECT_FALSE(redis->sismember("user:7:sessions", token));
}

TEST_F(SessionStoreTest, ASessionExpiresAfter30Days)
{
    SessionStore sessions(*redis);
    const std::string token = sessions.create("7");

    const long long ttl = redis->ttl("session:" + fingerprint(token));
    EXPECT_GT(ttl, 30LL * 24 * 3600 - 60);
    EXPECT_LE(ttl, 30LL * 24 * 3600);
}

TEST_F(SessionStoreTest, RemoveLogsOut)
{
    SessionStore sessions(*redis);
    const std::string token = sessions.create("7");
    sessions.remove(token);
    EXPECT_FALSE(sessions.userIdFor(token));
    EXPECT_FALSE(redis->exists("user:7:sessions"));
}

TEST_F(SessionStoreTest, RemoveAllExceptEndsTheOtherDevicesOfThatUserOnly)
{
    SessionStore sessions(*redis);
    const std::string laptop = sessions.create("7");
    const std::string phone = sessions.create("7");
    const std::string tablet = sessions.create("7");
    const std::string someoneElse = sessions.create("8");

    sessions.removeAllExcept("7", laptop);

    EXPECT_EQ(sessions.userIdFor(laptop).value(), "7");
    EXPECT_FALSE(sessions.userIdFor(phone));
    EXPECT_FALSE(sessions.userIdFor(tablet));
    EXPECT_EQ(sessions.userIdFor(someoneElse).value(), "8");
    EXPECT_EQ(redis->scard("user:7:sessions"), 1);
}

TEST_F(SessionStoreTest, UnknownOrMalformedTokensHaveNoUser)
{
    SessionStore sessions(*redis);
    EXPECT_FALSE(sessions.userIdFor(""));
    EXPECT_FALSE(sessions.userIdFor("abc"));
    EXPECT_FALSE(sessions.userIdFor(std::string(64, 'a')));
}

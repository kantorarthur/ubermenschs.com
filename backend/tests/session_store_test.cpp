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
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0], "session:" + fingerprint(token));
    EXPECT_EQ(keys[0].find(token), std::string::npos);
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
}

TEST_F(SessionStoreTest, UnknownOrMalformedTokensHaveNoUser)
{
    SessionStore sessions(*redis);
    EXPECT_FALSE(sessions.userIdFor(""));
    EXPECT_FALSE(sessions.userIdFor("abc"));
    EXPECT_FALSE(sessions.userIdFor(std::string(64, 'a')));
}

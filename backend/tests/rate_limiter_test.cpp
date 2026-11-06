#include "errors.hpp"
#include "rate_limiter.hpp"
#include "redis_fixture.hpp"

#include <chrono>

namespace
{
    const RateLimit THREE_PER_MINUTE{"test", 3, std::chrono::seconds(60)};

    class RateLimiterTest : public RedisTest
    {
    };
}

TEST_F(RateLimiterTest, AllowsUpToTheLimitThenRefuses)
{
    RateLimiter limiter(*redis);
    EXPECT_NO_THROW(limiter.hit(THREE_PER_MINUTE, "1.2.3.4"));
    EXPECT_NO_THROW(limiter.hit(THREE_PER_MINUTE, "1.2.3.4"));
    EXPECT_NO_THROW(limiter.hit(THREE_PER_MINUTE, "1.2.3.4"));
    EXPECT_THROW(limiter.hit(THREE_PER_MINUTE, "1.2.3.4"), TooManyRequestsError);
}

TEST_F(RateLimiterTest, TheRefusalSaysWhenToTryAgain)
{
    RateLimiter limiter(*redis);
    for(int i = 0; i < 3; ++i)
        limiter.hit(THREE_PER_MINUTE, "1.2.3.4");

    try
    {
        limiter.hit(THREE_PER_MINUTE, "1.2.3.4");
        FAIL() << "the fourth hit must be refused";
    }
    catch(const TooManyRequestsError& error)
    {
        EXPECT_GT(error.retryAfterSeconds(), 0);
        EXPECT_LE(error.retryAfterSeconds(), 60);
        EXPECT_STREQ(error.what(), "Too many attempts. Try again in a minute.");
    }
}

TEST_F(RateLimiterTest, SubjectsAreCountedSeparately)
{
    RateLimiter limiter(*redis);
    for(int i = 0; i < 3; ++i)
        limiter.hit(THREE_PER_MINUTE, "1.2.3.4");

    EXPECT_NO_THROW(limiter.hit(THREE_PER_MINUTE, "5.6.7.8"));
}

TEST_F(RateLimiterTest, TheCounterExpiresWithTheWindow)
{
    RateLimiter limiter(*redis);
    limiter.hit(THREE_PER_MINUTE, "1.2.3.4");

    const long long ttl = redis->ttl("ratelimit:test:1.2.3.4");
    EXPECT_GT(ttl, 0);
    EXPECT_LE(ttl, 60);
}

TEST_F(RateLimiterTest, ACounterWithoutExpiryGetsOne)
{
    // As if an older version had crashed between INCR and EXPIRE.
    redis->set("ratelimit:test:1.2.3.4", "1");
    ASSERT_EQ(redis->ttl("ratelimit:test:1.2.3.4"), -1);

    RateLimiter limiter(*redis);
    limiter.hit(THREE_PER_MINUTE, "1.2.3.4");
    EXPECT_GT(redis->ttl("ratelimit:test:1.2.3.4"), 0);
}

TEST_F(RateLimiterTest, CheckDoesNotCountAHit)
{
    RateLimiter limiter(*redis);
    for(int i = 0; i < 10; ++i)
        limiter.check(THREE_PER_MINUTE, "1.2.3.4");

    EXPECT_FALSE(redis->exists("ratelimit:test:1.2.3.4"));
}

TEST_F(RateLimiterTest, CheckRefusesOnceTheLimitIsUsedUp)
{
    RateLimiter limiter(*redis);
    for(int i = 0; i < 3; ++i)
        limiter.hit(THREE_PER_MINUTE, "ana@example.com");

    EXPECT_THROW(limiter.check(THREE_PER_MINUTE, "ana@example.com"), TooManyRequestsError);
}

TEST_F(RateLimiterTest, ResetForgetsTheHits)
{
    RateLimiter limiter(*redis);
    for(int i = 0; i < 3; ++i)
        limiter.hit(THREE_PER_MINUTE, "ana@example.com");

    limiter.reset(THREE_PER_MINUTE, "ana@example.com");
    EXPECT_NO_THROW(limiter.check(THREE_PER_MINUTE, "ana@example.com"));
    EXPECT_NO_THROW(limiter.hit(THREE_PER_MINUTE, "ana@example.com"));
}

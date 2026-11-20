#include "config.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(RedisUrl, TheInternalUrlRenderGives)
{
    Config config{};
    applyRedisUrl("redis://red-abc123xyz:6379", config);
    EXPECT_EQ(config.redisHost, "red-abc123xyz");
    EXPECT_EQ(config.redisPort, 6379);
    EXPECT_EQ(config.redisUser, "");
    EXPECT_EQ(config.redisPassword, "");
}

TEST(RedisUrl, WithUserAndPassword)
{
    Config config{};
    applyRedisUrl("redis://red-user:s3cr:et@red-abc123xyz:6380/0", config);
    EXPECT_EQ(config.redisUser, "red-user");
    EXPECT_EQ(config.redisPassword, "s3cr:et");
    EXPECT_EQ(config.redisHost, "red-abc123xyz");
    EXPECT_EQ(config.redisPort, 6380);
}

TEST(RedisUrl, PasswordOnlyAndDefaultPort)
{
    Config config{};
    applyRedisUrl("redis://:hunter2@localhost", config);
    EXPECT_EQ(config.redisUser, "");
    EXPECT_EQ(config.redisPassword, "hunter2");
    EXPECT_EQ(config.redisHost, "localhost");
    EXPECT_EQ(config.redisPort, 6379);
}

TEST(RedisUrl, BadUrlsAreRefused)
{
    Config config{};
    EXPECT_THROW(applyRedisUrl("rediss://red-abc:6379", config), std::runtime_error);
    EXPECT_THROW(applyRedisUrl("http://red-abc:6379", config), std::runtime_error);
    EXPECT_THROW(applyRedisUrl("redis://:6379", config), std::runtime_error);
    EXPECT_THROW(applyRedisUrl("redis://red-abc:port", config), std::runtime_error);
    EXPECT_THROW(applyRedisUrl("redis://red-abc:70000", config), std::runtime_error);
}

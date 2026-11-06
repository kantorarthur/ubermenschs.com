#pragma once

#include "crypto.hpp"

#include <gtest/gtest.h>
#include <sw/redis++/redis++.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>

// Base for tests that need a real Redis.
//
// It connects to REDIS_HOST:REDIS_PORT (default 127.0.0.1:6379) and uses logical database
// TEST_REDIS_DB (default 15), which is emptied before every test. The site's data lives in
// database 0, so the tests never touch it; database 0 is refused on purpose.
// If Redis cannot be reached, these tests are skipped instead of failing.
class RedisTest : public testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        ASSERT_TRUE(initCrypto());
    }

    void SetUp() override
    {
        sw::redis::ConnectionOptions options;
        options.host = env("REDIS_HOST", "127.0.0.1");
        options.port = std::stoi(env("REDIS_PORT", "6379"));
        options.db = std::stoi(env("TEST_REDIS_DB", "15"));
        options.connect_timeout = std::chrono::milliseconds(500);
        options.socket_timeout = std::chrono::milliseconds(2000);

        ASSERT_NE(options.db, 0) << "The tests empty their database; never point them at database 0.";

        redis = std::make_unique<sw::redis::Redis>(options);
        try
        {
            redis->ping();
        }
        catch(const sw::redis::Error& error)
        {
            GTEST_SKIP() << "Redis is not reachable (" << error.what() << ")";
        }
        redis->flushdb();
    }

    std::unique_ptr<sw::redis::Redis> redis;

private:
    static std::string env(const char* name, const char* fallback)
    {
        const char* value = std::getenv(name);
        return value != nullptr && *value != '\0' ? value : fallback;
    }
};

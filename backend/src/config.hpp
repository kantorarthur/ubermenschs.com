#pragma once

#include <string>

// Settings read from environment variables, so the same program runs
// locally, in docker compose and on Render without being rebuilt.
struct Config
{
    int port;                  // PORT           (default 8080)
    std::string redisHost;     // REDIS_HOST     (default 127.0.0.1)
    int redisPort;             // REDIS_PORT     (default 6379)
    std::string redisPassword; // REDIS_PASSWORD (default: none)
};

// Reads the configuration; throws std::runtime_error when a value is invalid.
Config loadConfig();

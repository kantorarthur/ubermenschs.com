#include "config.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace
{
    std::string envOr(const char* name, const std::string& fallback)
    {
        const char* value = std::getenv(name);
        if(value == nullptr || *value == '\0')
            return fallback;
        return value;
    }

    int portFromEnv(const char* name, int fallback)
    {
        const std::string text = envOr(name, std::to_string(fallback));

        int port = 0;
        try
        {
            port = std::stoi(text);
        }
        catch(const std::exception&)
        {
            throw std::runtime_error(std::string(name) + " must be a number, got \"" + text + "\"");
        }

        if(port < 1 || port > 65535)
            throw std::runtime_error(std::string(name) + " must be between 1 and 65535");
        return port;
    }
}

Config loadConfig()
{
    Config config;
    config.port = portFromEnv("PORT", 8080);
    config.redisHost = envOr("REDIS_HOST", "127.0.0.1");
    config.redisPort = portFromEnv("REDIS_PORT", 6379);
    config.redisPassword = envOr("REDIS_PASSWORD", "");

    const std::string secure = envOr("COOKIE_SECURE", "false");
    config.cookieSecure = secure == "true" || secure == "1";
    config.cookieDomain = envOr("COOKIE_DOMAIN", "");

    const std::string proxy = envOr("TRUST_PROXY", "false");
    config.trustProxy = proxy == "true" || proxy == "1";
    return config;
}

#include "config.hpp"

#include "push_store.hpp"

#include <cstdlib>
#include <stdexcept>
#include <sstream>
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

    // "a.com, b.com" → {"a.com", "b.com"}
    std::vector<std::string> splitList(const std::string& text)
    {
        std::vector<std::string> items;
        std::stringstream stream(text);
        std::string item;
        while(std::getline(stream, item, ','))
        {
            const std::size_t first = item.find_first_not_of(' ');
            const std::size_t last = item.find_last_not_of(' ');
            if(first != std::string::npos)
                items.push_back(item.substr(first, last - first + 1));
        }
        return items;
    }
}

void applyRedisUrl(const std::string& url, Config& config)
{
    const std::string scheme = "redis://";
    if(url.rfind(scheme, 0) != 0)
        throw std::runtime_error("REDIS_URL must start with redis:// (rediss:// is not supported)");

    std::string rest = url.substr(scheme.size());
    // A database number or options after "/" are not used.
    rest = rest.substr(0, rest.find('/'));

    const std::size_t at = rest.rfind('@');
    if(at != std::string::npos)
    {
        const std::string credentials = rest.substr(0, at);
        const std::size_t colon = credentials.find(':');
        if(colon == std::string::npos)
        {
            config.redisPassword = credentials;
        }
        else
        {
            if(colon > 0)
                config.redisUser = credentials.substr(0, colon);
            config.redisPassword = credentials.substr(colon + 1);
        }
        rest = rest.substr(at + 1);
    }

    const std::size_t colon = rest.rfind(':');
    config.redisHost = rest.substr(0, colon);
    if(config.redisHost.empty())
        throw std::runtime_error("REDIS_URL has no host");
    if(colon != std::string::npos)
    {
        try
        {
            config.redisPort = std::stoi(rest.substr(colon + 1));
        }
        catch(const std::exception&)
        {
            throw std::runtime_error("REDIS_URL has an invalid port");
        }
        if(config.redisPort < 1 || config.redisPort > 65535)
            throw std::runtime_error("REDIS_URL has an invalid port");
    }
    else
    {
        config.redisPort = 6379;
    }
}

Config loadConfig()
{
    Config config;
    config.port = portFromEnv("PORT", 8080);
    config.redisHost = envOr("REDIS_HOST", "127.0.0.1");
    config.redisPort = portFromEnv("REDIS_PORT", 6379);
    config.redisPassword = envOr("REDIS_PASSWORD", "");
    config.redisUser = "";
    const std::string redisUrl = envOr("REDIS_URL", "");
    if(!redisUrl.empty())
        applyRedisUrl(redisUrl, config);
    const std::string replicas = envOr("REDIS_REPLICAS", "0");
    if(replicas != "0" && replicas != "1" && replicas != "2")
        throw std::runtime_error("REDIS_REPLICAS must be 0, 1 or 2, got \"" + replicas + "\"");
    config.redisReplicas = std::stoi(replicas);

    const std::string secure = envOr("COOKIE_SECURE", "false");
    config.cookieSecure = secure == "true" || secure == "1";
    config.cookieDomain = envOr("COOKIE_DOMAIN", "");

    const std::string proxy = envOr("TRUST_PROXY", "false");
    config.trustProxy = proxy == "true" || proxy == "1";
    config.allowedOrigins = splitList(envOr("ALLOWED_ORIGINS", ""));

    config.vapidPublicKey = envOr("VAPID_PUBLIC_KEY", "");
    config.vapidPrivateKey = envOr("VAPID_PRIVATE_KEY", "");
    config.vapidSubject = envOr("VAPID_SUBJECT", "https://ubermenschs.com");
    const std::string hosts = envOr("PUSH_ALLOWED_HOSTS", "");
    config.pushAllowedHosts = hosts.empty() ? DEFAULT_PUSH_HOSTS : splitList(hosts);
    return config;
}

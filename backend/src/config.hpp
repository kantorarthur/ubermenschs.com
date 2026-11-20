#pragma once

#include <string>
#include <vector>

// Settings read from environment variables, so the same program runs
// locally, in docker compose and on Render without being rebuilt.
struct Config
{
    int port;                  // PORT           (default 8080)
    std::string redisHost;     // REDIS_HOST     (default 127.0.0.1)
    int redisPort;             // REDIS_PORT     (default 6379)
    std::string redisUser;     // (only from REDIS_URL) the Redis ACL user; default "default"
    std::string redisPassword; // REDIS_PASSWORD (default: none)
    int redisReplicas;         // REDIS_REPLICAS how many replicas must confirm important writes (WAIT);
                               //                0 (default) skips the wait; docker compose sets 1
    bool cookieSecure;         // COOKIE_SECURE  "true" in production (HTTPS only); default false for localhost
    std::string cookieDomain;  // COOKIE_DOMAIN  e.g. ".ubermenschs.com" in production; default: none
    bool trustProxy;           // TRUST_PROXY    "true" behind Render's proxy: the client IP is in X-Forwarded-For
    std::vector<std::string> allowedOrigins; // ALLOWED_ORIGINS  sites that may call the API from the browser (CORS),
                                             //                  e.g. "https://ubermenschs.com"; default: none

    // Push notifications (see push_sender.hpp)
    std::string vapidPublicKey;  // VAPID_PUBLIC_KEY   base64url; without it a key pair is made and kept in Redis
    std::string vapidPrivateKey; // VAPID_PRIVATE_KEY  base64url; a secret, like a password
    std::string vapidSubject;    // VAPID_SUBJECT      how push services can contact us (default https://ubermenschs.com)
    std::vector<std::string> pushAllowedHosts; // PUSH_ALLOWED_HOSTS  comma-separated; default: the big browsers' services
};

// Reads the configuration; throws std::runtime_error when a value is invalid.
// REDIS_URL ("redis://[user:password@]host[:port]", what Render gives for Key Value), if set,
// replaces REDIS_HOST, REDIS_PORT and REDIS_PASSWORD.
Config loadConfig();

// Fills redisHost, redisPort, redisUser and redisPassword from a redis:// URL.
// Throws std::runtime_error for anything else (rediss:// with TLS is not supported by this build).
void applyRedisUrl(const std::string& url, Config& config);

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
    std::string redisPassword; // REDIS_PASSWORD (default: none)
    bool cookieSecure;         // COOKIE_SECURE  "true" in production (HTTPS only); default false for localhost
    std::string cookieDomain;  // COOKIE_DOMAIN  e.g. ".ubermenschs.com" in production; default: none
    bool trustProxy;           // TRUST_PROXY    "true" behind Render's proxy: the client IP is in X-Forwarded-For

    // Push notifications (see push_sender.hpp)
    std::string vapidPublicKey;  // VAPID_PUBLIC_KEY   base64url; without it a key pair is made and kept in Redis
    std::string vapidPrivateKey; // VAPID_PRIVATE_KEY  base64url; a secret, like a password
    std::string vapidSubject;    // VAPID_SUBJECT      how push services can contact us (default https://ubermenschs.com)
    std::vector<std::string> pushAllowedHosts; // PUSH_ALLOWED_HOSTS  comma-separated; default: the big browsers' services
};

// Reads the configuration; throws std::runtime_error when a value is invalid.
Config loadConfig();

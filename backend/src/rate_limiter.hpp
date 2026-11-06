#pragma once

#include <sw/redis++/redis++.h>

#include <chrono>
#include <string>

// A rule: at most `maxHits` within `window`, counted per subject (an IP address, an email, ...).
struct RateLimit
{
    std::string name;
    long long maxHits;
    std::chrono::seconds window;
};

// The limits used by the site, all in one place.
namespace rateLimits
{
    // Every login attempt from one IP address, right or wrong.
    inline const RateLimit loginPerIp{"login-ip", 20, std::chrono::minutes(15)};
    // Wrong passwords for one account, from anywhere: stops guessing one person's password
    // even with many IP addresses. A successful login clears it.
    inline const RateLimit loginFailuresPerEmail{"login-fail", 5, std::chrono::minutes(15)};
    // New accounts from one IP address.
    inline const RateLimit registerPerIp{"register-ip", 5, std::chrono::hours(1)};
    // Test notifications (the button on the Reminder page) of one user.
    inline const RateLimit pushTestPerUser{"push-test", 5, std::chrono::minutes(10)};
}

// Fixed-window counters in Redis:
//
//   ratelimit:{limit name}:{subject}   a number that counts hits; it expires when its window ends
//
// The first hit creates the key with the window as its lifetime, the next ones only increase it.
// When the key expires, counting starts again from zero.
class RateLimiter
{
public:
    explicit RateLimiter(sw::redis::Redis& redis);

    // Counts one hit. Throws TooManyRequestsError (HTTP 429) if this hit goes over the limit.
    void hit(const RateLimit& limit, const std::string& subject);

    // Throws TooManyRequestsError if the limit is already used up, without counting a hit.
    void check(const RateLimit& limit, const std::string& subject);

    // Forgets the hits of a subject (e.g. failed logins after a successful one).
    void reset(const RateLimit& limit, const std::string& subject);

private:
    sw::redis::Redis& redis_;
};

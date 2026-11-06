#include "rate_limiter.hpp"

#include "errors.hpp"

#include <iterator>
#include <string>
#include <vector>

namespace
{
    // Runs inside Redis as one step, so no other request can come between INCR and EXPIRE.
    // (Done as two separate commands, a crash in between would leave a counter that never
    // expires, and that subject would be blocked forever.)
    // Returns {hits so far, seconds until the window ends}.
    const char* const HIT_SCRIPT = R"lua(
        local hits = redis.call('INCR', KEYS[1])
        local ttl = redis.call('TTL', KEYS[1])
        if ttl < 0 then
            redis.call('EXPIRE', KEYS[1], ARGV[1])
            ttl = tonumber(ARGV[1])
        end
        return {hits, ttl}
    )lua";

    std::string counterKey(const RateLimit& limit, const std::string& subject)
    {
        return "ratelimit:" + limit.name + ":" + subject;
    }

    [[noreturn]] void throwTooMany(long long retryAfterSeconds)
    {
        const long long minutes = (retryAfterSeconds + 59) / 60;
        const std::string when = minutes <= 1 ? "a minute" : std::to_string(minutes) + " minutes";
        throw TooManyRequestsError("Too many attempts. Try again in " + when + ".", retryAfterSeconds);
    }
}

RateLimiter::RateLimiter(sw::redis::Redis& redis)
    : redis_(redis)
{
}

void RateLimiter::hit(const RateLimit& limit, const std::string& subject)
{
    const std::string key = counterKey(limit, subject);
    const std::string windowSeconds = std::to_string(limit.window.count());

    std::vector<long long> reply;
    redis_.eval(HIT_SCRIPT, {key}, {windowSeconds}, std::back_inserter(reply));

    if(reply.size() == 2 && reply[0] > limit.maxHits)
        throwTooMany(reply[1]);
}

void RateLimiter::check(const RateLimit& limit, const std::string& subject)
{
    const std::string key = counterKey(limit, subject);
    const auto hits = redis_.get(key);
    if(!hits)
        return;

    long long count = 0;
    try
    {
        count = std::stoll(*hits);
    }
    catch(const std::exception&)
    {
        return;
    }

    if(count >= limit.maxHits)
    {
        const long long ttl = redis_.ttl(key);
        throwTooMany(ttl > 0 ? ttl : limit.window.count());
    }
}

void RateLimiter::reset(const RateLimit& limit, const std::string& subject)
{
    redis_.del(counterKey(limit, subject));
}

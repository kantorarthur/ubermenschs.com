#include "push_store.hpp"

#include "crypto.hpp"
#include "errors.hpp"
#include "redis_client.hpp"
#include "webpush_crypto.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace
{
    std::string subscriptionKey(const std::string& subscriptionId)
    {
        return "push:sub:" + subscriptionId;
    }

    std::string userPushKey(const std::string& userId)
    {
        return "user:" + userId + ":push";
    }

    struct ParsedEndpoint
    {
        std::string scheme; // "https" or "http"
        std::string host;   // lower case, without the port
        std::string origin; // scheme://host[:port]
    };

    // Splits "https://host[:port]/path" into its parts; returns false if it does not look like that.
    bool parseEndpoint(const std::string& endpoint, ParsedEndpoint& parsed)
    {
        const std::size_t schemeEnd = endpoint.find("://");
        if(schemeEnd == std::string::npos)
            return false;
        parsed.scheme = endpoint.substr(0, schemeEnd);

        const std::size_t authorityStart = schemeEnd + 3;
        const std::size_t authorityEnd = endpoint.find_first_of("/?#", authorityStart);
        const std::string authority = endpoint.substr(authorityStart, authorityEnd - authorityStart);
        // "user@host" could hide the real host from a careless reader; push endpoints never have it.
        if(authority.empty() || authority.find('@') != std::string::npos)
            return false;

        const std::size_t colon = authority.find(':');
        parsed.host = authority.substr(0, colon);
        if(colon != std::string::npos)
        {
            const std::string port = authority.substr(colon + 1);
            const bool digits = std::all_of(port.begin(), port.end(), [](unsigned char c)
            {
                return std::isdigit(c) != 0;
            });
            if(port.empty() || port.size() > 5 || !digits)
                return false;
        }
        std::transform(parsed.host.begin(), parsed.host.end(), parsed.host.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        parsed.origin = parsed.scheme + "://" + authority;
        return !parsed.host.empty();
    }

    bool hostMatches(const std::string& host, const std::string& allowed)
    {
        if(host == allowed)
            return true;
        // A subdomain: "web.push.apple.com" for "push.apple.com", but not "evilpush.apple.com.example".
        return host.size() > allowed.size()
            && host.compare(host.size() - allowed.size(), allowed.size(), allowed) == 0
            && host[host.size() - allowed.size() - 1] == '.';
    }

    std::string decodeKey(const std::string& text, const std::string& name)
    {
        try
        {
            return base64UrlDecode(text);
        }
        catch(const std::invalid_argument&)
        {
            throw ValidationError("The subscription's " + name + " key is not valid.");
        }
    }
}

bool isAllowedPushEndpoint(const std::string& endpoint, const std::vector<std::string>& allowedHosts)
{
    ParsedEndpoint parsed;
    if(endpoint.size() > limits::pushEndpoint || !parseEndpoint(endpoint, parsed))
        return false;

    const bool local = parsed.host == "localhost" || parsed.host == "127.0.0.1";
    if(parsed.scheme != "https" && !(parsed.scheme == "http" && local))
        return false;

    return std::any_of(allowedHosts.begin(), allowedHosts.end(), [&parsed](const std::string& allowed)
    {
        return hostMatches(parsed.host, allowed);
    });
}

std::string pushOrigin(const std::string& endpoint)
{
    ParsedEndpoint parsed;
    if(!parseEndpoint(endpoint, parsed))
        throw std::invalid_argument("not an endpoint: " + endpoint);
    return parsed.origin;
}

PushStore::PushStore(sw::redis::Redis& redis, std::vector<std::string> allowedHosts)
    : redis_(redis), allowedHosts_(std::move(allowedHosts))
{
}

std::string PushStore::save(const std::string& userId, const std::string& endpoint, const std::string& p256dh,
                            const std::string& auth)
{
    if(!isAllowedPushEndpoint(endpoint, allowedHosts_))
        throw ValidationError("This browser's push service is not supported.");
    if(!isP256PublicKey(decodeKey(p256dh, "p256dh")))
        throw ValidationError("The subscription's p256dh key is not valid.");
    if(decodeKey(auth, "auth").size() != 16)
        throw ValidationError("The subscription's auth key is not valid.");

    const std::string id = fingerprint(endpoint);
    const auto previousOwner = redis_.hget(subscriptionKey(id), "userId");
    const bool isNew = !previousOwner || *previousOwner != userId;
    if(isNew && static_cast<std::size_t>(redis_.scard(userPushKey(userId))) >= limits::pushSubscriptionsPerUser)
        throw ValidationError("Notifications can be on for at most "
                              + std::to_string(limits::pushSubscriptionsPerUser) + " browsers.");

    const std::vector<std::pair<std::string, std::string>> fields = {
        {"userId", userId},
        {"endpoint", endpoint},
        {"p256dh", p256dh},
        {"auth", auth},
    };

    auto transaction = redis_.transaction();
    if(previousOwner && *previousOwner != userId)
        transaction.srem(userPushKey(*previousOwner), id);
    transaction.hset(subscriptionKey(id), fields.begin(), fields.end())
               .sadd(userPushKey(userId), id);
    transaction.exec();
    waitForReplica(transaction.redis());
    return id;
}

void PushStore::remove(const std::string& userId, const std::string& endpoint)
{
    const std::string id = fingerprint(endpoint);
    const auto owner = redis_.hget(subscriptionKey(id), "userId");
    if(!owner || *owner != userId)
        return;

    auto transaction = redis_.transaction();
    transaction.del(subscriptionKey(id))
               .srem(userPushKey(userId), id);
    transaction.exec();
}

void PushStore::removeById(const std::string& subscriptionId)
{
    const auto owner = redis_.hget(subscriptionKey(subscriptionId), "userId");

    auto transaction = redis_.transaction();
    transaction.del(subscriptionKey(subscriptionId));
    if(owner)
        transaction.srem(userPushKey(*owner), subscriptionId);
    transaction.exec();
}

std::vector<PushSubscription> PushStore::list(const std::string& userId)
{
    std::vector<std::string> ids;
    redis_.smembers(userPushKey(userId), std::back_inserter(ids));

    std::vector<PushSubscription> subscriptions;
    for(const std::string& id : ids)
    {
        std::unordered_map<std::string, std::string> fields;
        redis_.hgetall(subscriptionKey(id), std::inserter(fields, fields.begin()));
        if(fields.empty() || fields["userId"] != userId)
            continue;

        subscriptions.push_back(PushSubscription{id, userId, fields["endpoint"], fields["p256dh"], fields["auth"]});
    }
    return subscriptions;
}

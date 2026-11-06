#pragma once

#include <sw/redis++/redis++.h>

#include <cstddef>
#include <string>
#include <vector>

// One browser on one device that agreed to get notifications. The browser gives these three
// values when it subscribes (PushSubscription.toJSON() in JavaScript).
struct PushSubscription
{
    std::string id;       // BLAKE2b fingerprint of the endpoint
    std::string userId;
    std::string endpoint; // the push service's address for this browser, e.g. https://fcm.googleapis.com/fcm/send/...
    std::string p256dh;   // the browser's public key, base64url
    std::string auth;     // the browser's 16-byte secret, base64url
};

namespace limits
{
    inline constexpr std::size_t pushSubscriptionsPerUser = 10;
    inline constexpr std::size_t pushEndpoint = 2048;
}

// The push services of the big browsers. The server sends requests to the endpoint a browser gives,
// so only these hosts (and their subdomains) are accepted; otherwise anyone could make our server
// send requests to any address.
inline const std::vector<std::string> DEFAULT_PUSH_HOSTS = {
    "fcm.googleapis.com",               // Chrome, Edge on Android, Opera, Brave, Samsung Internet
    "updates.push.services.mozilla.com", // Firefox
    "push.apple.com",                   // Safari (macOS, and iOS when added to the home screen)
    "notify.windows.com",               // Edge on Windows
};

// True for an https:// address whose host is one of allowedHosts or a subdomain of one.
// http:// is accepted only for localhost and 127.0.0.1, and only if they are in allowedHosts (for tests).
bool isAllowedPushEndpoint(const std::string& endpoint, const std::vector<std::string>& allowedHosts);

// "https://fcm.googleapis.com" for "https://fcm.googleapis.com/fcm/send/abc": the "aud" of the VAPID JWT.
std::string pushOrigin(const std::string& endpoint);

// The browsers that get a user's notifications, in Redis:
//
//   push:sub:{id}     hash: userId, endpoint, p256dh, auth   ({id} is a fingerprint of the endpoint)
//   user:{id}:push    set of the user's subscription ids
//
// One browser has one endpoint, so subscribing again replaces the old entry; if another account
// logs in on the same browser and subscribes, the browser moves to that account.
class PushStore
{
public:
    PushStore(sw::redis::Redis& redis, std::vector<std::string> allowedHosts);

    // Checks and saves a subscription; returns its id. Throws ValidationError for bad data.
    std::string save(const std::string& userId, const std::string& endpoint, const std::string& p256dh,
                     const std::string& auth);

    // Forgets the subscription with this endpoint if it belongs to the user; does nothing otherwise.
    void remove(const std::string& userId, const std::string& endpoint);

    // Forgets a subscription, e.g. when its push service says it no longer exists.
    void removeById(const std::string& subscriptionId);

    std::vector<PushSubscription> list(const std::string& userId);

private:
    sw::redis::Redis& redis_;
    std::vector<std::string> allowedHosts_;
};

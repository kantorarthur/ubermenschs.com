#pragma once

#include "push_store.hpp"

#include <sw/redis++/redis++.h>

#include <mutex>
#include <optional>
#include <string>

enum class PushResult
{
    Delivered, // the push service accepted the notification
    Gone,      // the browser unsubscribed or was uninstalled (404/410): forget the subscription
    Failed,    // anything else; try again next time
};

// Sends one notification to one browser. An interface, so the scheduler can be tested without the internet.
class PushSender
{
public:
    virtual ~PushSender() = default;
    virtual PushResult send(const PushSubscription& subscription, const std::string& payload) = 0;
};

// The server's VAPID key pair (raw bytes). Browsers get the public key when they subscribe and the
// push services only accept notifications signed with the private key, so the pair must never change:
// a new pair makes every existing subscription useless.
struct VapidKeys
{
    std::string publicKey;
    std::string privateKey;
};

// Where the VAPID keys come from:
//   - VAPID_PUBLIC_KEY and VAPID_PRIVATE_KEY (base64url), if set: the way to go in production;
//   - otherwise a pair made on first use and kept in Redis under "vapid:keys" (good for local development).
class VapidKeyStore
{
public:
    // Throws std::runtime_error if the keys from the environment are not a valid pair.
    VapidKeyStore(sw::redis::Redis& redis, const std::string& publicKey, const std::string& privateKey);

    // Thread-safe. Throws sw::redis::Error if the keys have to come from Redis and Redis is down.
    VapidKeys keys();

private:
    sw::redis::Redis& redis_;
    std::mutex mutex_;
    std::optional<VapidKeys> keys_;
};

// Sends real notifications with the Web Push protocol (RFC 8030): an HTTPS POST to the browser's
// endpoint with the encrypted message (RFC 8291) and a VAPID signature (RFC 8292), using libcurl.
class WebPushSender : public PushSender
{
public:
    // subject: how a push service can contact us, "mailto:..." or "https://...".
    WebPushSender(VapidKeyStore& keys, std::string subject);

    PushResult send(const PushSubscription& subscription, const std::string& payload) override;

private:
    VapidKeyStore& keys_;
    std::string subject_;
};

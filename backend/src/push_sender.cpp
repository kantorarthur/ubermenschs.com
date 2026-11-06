#include "push_sender.hpp"

#include "webpush_crypto.hpp"

#include <crow/logging.h>
#include <curl/curl.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    // How long a push service keeps a notification for a phone that is offline (seconds).
    // A reminder that arrives hours late is no use, so one hour.
    constexpr long TIME_TO_LIVE = 3600;

    const std::string REDIS_KEY = "vapid:keys";

    VapidKeys decodePair(const std::string& publicKey, const std::string& privateKey)
    {
        VapidKeys keys{base64UrlDecode(publicKey), base64UrlDecode(privateKey)};
        if(p256PublicKeyOf(keys.privateKey) != keys.publicKey)
            throw std::invalid_argument("the public key does not belong to the private key");
        return keys;
    }

    // Collects the first kilobyte of the push service's answer, for the log.
    std::size_t collect(char* data, std::size_t size, std::size_t count, void* target)
    {
        auto* text = static_cast<std::string*>(target);
        const std::size_t length = size * count;
        if(text->size() < 1024)
            text->append(data, std::min(length, 1024 - text->size()));
        return length;
    }

    void initCurlOnce()
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        });
    }
}

VapidKeyStore::VapidKeyStore(sw::redis::Redis& redis, const std::string& publicKey, const std::string& privateKey)
    : redis_(redis)
{
    if(publicKey.empty() && privateKey.empty())
        return;
    if(publicKey.empty() || privateKey.empty())
        throw std::runtime_error("set both VAPID_PUBLIC_KEY and VAPID_PRIVATE_KEY, or neither");

    try
    {
        keys_ = decodePair(publicKey, privateKey);
    }
    catch(const std::invalid_argument& error)
    {
        throw std::runtime_error(std::string("VAPID_PUBLIC_KEY / VAPID_PRIVATE_KEY: ") + error.what());
    }
}

VapidKeys VapidKeyStore::keys()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(keys_)
        return *keys_;

    // SET NX: if several servers start at once, only the first pair is kept and all of them use it.
    const P256KeyPair fresh = generateP256KeyPair();
    const std::string stored = base64UrlEncode(fresh.publicKey) + "." + base64UrlEncode(fresh.privateKey);
    if(redis_.set(REDIS_KEY, stored, std::chrono::milliseconds(0), sw::redis::UpdateType::NOT_EXIST))
    {
        CROW_LOG_WARNING << "No VAPID_PUBLIC_KEY / VAPID_PRIVATE_KEY set: made a new key pair and saved it in "
                            "Redis (" << REDIS_KEY << "). Set the variables in production.";
    }

    const auto saved = redis_.get(REDIS_KEY);
    if(!saved)
        throw std::runtime_error("the VAPID keys disappeared from Redis");
    const std::size_t dot = saved->find('.');
    keys_ = decodePair(saved->substr(0, dot), saved->substr(dot + 1));
    return *keys_;
}

WebPushSender::WebPushSender(VapidKeyStore& keys, std::string subject)
    : keys_(keys), subject_(std::move(subject))
{
    initCurlOnce();
}

PushResult WebPushSender::send(const PushSubscription& subscription, const std::string& payload)
{
    std::string body;
    std::string authorization;
    try
    {
        const VapidKeys keys = keys_.keys();
        body = encryptPushPayload(payload, base64UrlDecode(subscription.p256dh), base64UrlDecode(subscription.auth));

        // The signature is valid for 12 hours (the maximum is 24).
        const auto now = std::chrono::system_clock::now();
        const long long expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
            (now + std::chrono::hours(12)).time_since_epoch()).count();
        authorization = "Authorization: vapid t="
                      + vapidJwt(pushOrigin(subscription.endpoint), expiresAt, subject_, keys.privateKey)
                      + ", k=" + base64UrlEncode(keys.publicKey);
    }
    catch(const std::invalid_argument& error)
    {
        // Saved data that cannot be used: the subscription will never work.
        CROW_LOG_WARNING << "Push subscription " << subscription.id << " is unusable: " << error.what();
        return PushResult::Gone;
    }

    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
    if(!curl)
        return PushResult::Failed;

    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("TTL: " + std::to_string(TIME_TO_LIVE)).c_str());
    headers = curl_slist_append(headers, "Urgency: high");
    headers = curl_slist_append(headers, "Content-Encoding: aes128gcm");
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, authorization.c_str());
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headerList(headers, &curl_slist_free_all);

    std::string answer;
    curl_easy_setopt(curl.get(), CURLOPT_URL, subscription.endpoint.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS_STR, "https,http");
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headerList.get());
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, collect);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &answer);

    const CURLcode code = curl_easy_perform(curl.get());
    if(code != CURLE_OK)
    {
        CROW_LOG_WARNING << "Push to " << pushOrigin(subscription.endpoint) << " failed: " << curl_easy_strerror(code);
        return PushResult::Failed;
    }

    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if(status >= 200 && status < 300)
        return PushResult::Delivered;
    if(status == 404 || status == 410)
        return PushResult::Gone;

    CROW_LOG_WARNING << "Push to " << pushOrigin(subscription.endpoint) << " answered " << status << ": " << answer;
    return PushResult::Failed;
}

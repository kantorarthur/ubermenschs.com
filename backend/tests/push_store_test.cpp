#include "errors.hpp"
#include "push_store.hpp"
#include "redis_fixture.hpp"
#include "webpush_crypto.hpp"

#include <iterator>
#include <string>
#include <vector>

namespace
{
    const std::string ANA = "1";
    const std::string BOB = "2";
    const std::string CHROME = "https://fcm.googleapis.com/fcm/send/abc123";
    const std::string FIREFOX = "https://updates.push.services.mozilla.com/wpush/v2/xyz";

    // Keys like the ones a browser sends.
    std::string browserPublicKey()
    {
        return base64UrlEncode(generateP256KeyPair().publicKey);
    }

    const std::string AUTH = base64UrlEncode("0123456789abcdef");

    class PushStoreTest : public RedisTest
    {
    };
}

TEST(PushEndpoint, OnlyTheKnownPushServices)
{
    const std::vector<std::string>& hosts = DEFAULT_PUSH_HOSTS;
    EXPECT_TRUE(isAllowedPushEndpoint(CHROME, hosts));
    EXPECT_TRUE(isAllowedPushEndpoint(FIREFOX, hosts));
    EXPECT_TRUE(isAllowedPushEndpoint("https://web.push.apple.com/QGuQyavXutnMH...", hosts));
    EXPECT_TRUE(isAllowedPushEndpoint("https://wns2-par02p.notify.windows.com/w/?token=abc", hosts));
    EXPECT_TRUE(isAllowedPushEndpoint("https://FCM.googleapis.com:443/fcm/send/abc", hosts));

    EXPECT_FALSE(isAllowedPushEndpoint("https://example.com/push", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("https://fcm.googleapis.com.example.com/x", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("https://evilfcm.googleapis.com/x", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("https://user@fcm.googleapis.com/x", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("http://fcm.googleapis.com/fcm/send/abc", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("https://fcm.googleapis.com:abc/x", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("fcm.googleapis.com/fcm/send/abc", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("https://fcm.googleapis.com/" + std::string(2100, 'a'), hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("http://localhost:9000/push", hosts));
}

TEST(PushEndpoint, LocalhostOnlyWhenAllowedAndNeverForOtherHostsOverHttp)
{
    const std::vector<std::string> hosts = {"localhost", "push.example"};
    EXPECT_TRUE(isAllowedPushEndpoint("http://localhost:9000/push/1", hosts));
    EXPECT_FALSE(isAllowedPushEndpoint("http://push.example/1", hosts));
    EXPECT_TRUE(isAllowedPushEndpoint("https://push.example/1", hosts));
}

TEST(PushEndpoint, Origin)
{
    EXPECT_EQ(pushOrigin(CHROME), "https://fcm.googleapis.com");
    EXPECT_EQ(pushOrigin("http://localhost:9000/push/1?x=1"), "http://localhost:9000");
}

TEST_F(PushStoreTest, SaveListAndRemove)
{
    PushStore store(*redis, DEFAULT_PUSH_HOSTS);
    const std::string key = browserPublicKey();
    const std::string id = store.save(ANA, CHROME, key, AUTH);
    store.save(ANA, FIREFOX, browserPublicKey(), AUTH);

    std::vector<PushSubscription> subscriptions = store.list(ANA);
    ASSERT_EQ(subscriptions.size(), 2u);

    // Subscribing again with the same endpoint replaces the entry.
    const std::string newKey = browserPublicKey();
    EXPECT_EQ(store.save(ANA, CHROME, newKey, AUTH), id);
    subscriptions = store.list(ANA);
    ASSERT_EQ(subscriptions.size(), 2u);
    for(const PushSubscription& s : subscriptions)
    {
        if(s.endpoint == CHROME)
        {
            EXPECT_EQ(s.p256dh, newKey);
        }
    }

    store.remove(ANA, CHROME);
    store.remove(ANA, FIREFOX);
    EXPECT_TRUE(store.list(ANA).empty());
    std::vector<std::string> keys;
    redis->keys("*", std::back_inserter(keys));
    EXPECT_TRUE(keys.empty());
}

TEST_F(PushStoreTest, BadSubscriptionsAreRefused)
{
    PushStore store(*redis, DEFAULT_PUSH_HOSTS);
    EXPECT_THROW(store.save(ANA, "https://example.com/push", browserPublicKey(), AUTH), ValidationError);
    EXPECT_THROW(store.save(ANA, CHROME, "not base64!", AUTH), ValidationError);
    EXPECT_THROW(store.save(ANA, CHROME, base64UrlEncode(std::string(65, '\x04')), AUTH), ValidationError);
    EXPECT_THROW(store.save(ANA, CHROME, browserPublicKey(), base64UrlEncode("short")), ValidationError);
    EXPECT_TRUE(store.list(ANA).empty());
}

TEST_F(PushStoreTest, TheBrowserMovesToTheAccountThatSubscribesLast)
{
    PushStore store(*redis, DEFAULT_PUSH_HOSTS);
    store.save(ANA, CHROME, browserPublicKey(), AUTH);
    store.save(BOB, CHROME, browserPublicKey(), AUTH);

    EXPECT_TRUE(store.list(ANA).empty());
    ASSERT_EQ(store.list(BOB).size(), 1u);

    // Ana cannot switch off Bob's browser.
    store.remove(ANA, CHROME);
    EXPECT_EQ(store.list(BOB).size(), 1u);
}

TEST_F(PushStoreTest, RemoveById)
{
    PushStore store(*redis, DEFAULT_PUSH_HOSTS);
    const std::string id = store.save(ANA, CHROME, browserPublicKey(), AUTH);
    store.removeById(id);
    EXPECT_TRUE(store.list(ANA).empty());
    EXPECT_FALSE(redis->exists("user:" + ANA + ":push"));
}

TEST_F(PushStoreTest, AtMostTenBrowsersPerUser)
{
    PushStore store(*redis, DEFAULT_PUSH_HOSTS);
    for(std::size_t i = 0; i < limits::pushSubscriptionsPerUser; ++i)
        store.save(ANA, CHROME + std::to_string(i), browserPublicKey(), AUTH);

    EXPECT_THROW(store.save(ANA, FIREFOX, browserPublicKey(), AUTH), ValidationError);
    // Renewing one that is already there still works.
    EXPECT_NO_THROW(store.save(ANA, CHROME + "0", browserPublicKey(), AUTH));
}

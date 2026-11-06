#include "auth_routes.hpp"
#include "config.hpp"
#include "crypto.hpp"
#include "event_routes.hpp"
#include "event_store.hpp"
#include "http.hpp"
#include "preset_routes.hpp"
#include "preset_store.hpp"
#include "push_routes.hpp"
#include "push_sender.hpp"
#include "push_store.hpp"
#include "rate_limiter.hpp"
#include "redis_client.hpp"
#include "reminder_routes.hpp"
#include "reminder_scheduler.hpp"
#include "reminder_store.hpp"
#include "user_store.hpp"
#include "webpush_crypto.hpp"

#include <crow.h>
#include <sw/redis++/redis++.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

namespace
{
    // GET /api/health
    // 200 when the server and Redis both answer, 503 when Redis does not.
    crow::response health(sw::redis::Redis& redis)
    {
        crow::json::wvalue body;
        body["server"] = "ok";

        try
        {
            const std::string reply = redis.ping();
            body["redis"] = reply == "PONG" ? "ok" : "unexpected reply";
            body["replicas"] = connectedReplicas(redis);
        }
        catch(const sw::redis::Error& error)
        {
            CROW_LOG_WARNING << "Redis health check failed: " << error.what();
            body["redis"] = "unreachable";
            return jsonResponse(503, body);
        }

        return jsonResponse(200, body);
    }
}

int main(int argc, char** argv)
{
    // "server --generate-vapid-keys" prints a new VAPID key pair for the environment variables.
    if(argc > 1 && std::string(argv[1]) == "--generate-vapid-keys")
    {
        const P256KeyPair keys = generateP256KeyPair();
        std::cout << "VAPID_PUBLIC_KEY=" << base64UrlEncode(keys.publicKey) << '\n'
                  << "VAPID_PRIVATE_KEY=" << base64UrlEncode(keys.privateKey) << '\n';
        return 0;
    }

    Config config;
    try
    {
        config = loadConfig();
    }
    catch(const std::exception& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << '\n';
        return 1;
    }

    std::unique_ptr<sw::redis::Redis> redis = makeRedisClient(config);
    std::unique_ptr<VapidKeyStore> vapidKeys;
    try
    {
        vapidKeys = std::make_unique<VapidKeyStore>(*redis, config.vapidPublicKey, config.vapidPrivateKey);
    }
    catch(const std::exception& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << '\n';
        return 1;
    }

    if(!initCrypto())
    {
        std::cerr << "libsodium could not be initialised\n";
        return 1;
    }

    UserStore users(*redis);
    SessionStore sessions(*redis);
    PresetStore presets(*redis);
    EventStore events(*redis);
    ReminderStore reminders(*redis);
    RateLimiter limiter(*redis);
    PushStore pushSubscriptions(*redis, config.pushAllowedHosts);
    WebPushSender pushSender(*vapidKeys, config.vapidSubject);
    ReminderScheduler scheduler(*redis, pushSubscriptions, pushSender);

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Info);

    CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::Get)([&redis]()
    {
        return health(*redis);
    });

    addAuthRoutes(app, users, sessions, limiter, config);
    addPresetRoutes(app, presets, sessions);
    addEventRoutes(app, events, sessions);
    addReminderRoutes(app, reminders, sessions);
    addPushRoutes(app, pushSubscriptions, *vapidKeys, scheduler, sessions, limiter);

    // Sends the reminders that are due, every 15 seconds, until the server stops.
    std::jthread reminderThread([&scheduler](std::stop_token stop)
    {
        while(!stop.stop_requested())
        {
            try
            {
                const int sent = scheduler.sendDue(ReminderStore::systemClock());
                if(sent > 0)
                    CROW_LOG_INFO << "Sent " << sent << " reminder notification(s)";
            }
            catch(const std::exception& error)
            {
                CROW_LOG_WARNING << "Sending reminders failed: " << error.what();
            }

            for(int second = 0; second < 15 && !stop.stop_requested(); ++second)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    CROW_LOG_INFO << "Redis at " << config.redisHost << ':' << config.redisPort;

    // Listens on all network interfaces, as Docker and Render need.
    // Crow stops cleanly on Ctrl+C and on the SIGTERM sent by "docker compose down".
    app.bindaddr("0.0.0.0").port(static_cast<std::uint16_t>(config.port)).multithreaded().run();

    // Crow has stopped: stop the reminder thread too (jthread waits for it when it goes out of scope).
    reminderThread.request_stop();
    return 0;
}

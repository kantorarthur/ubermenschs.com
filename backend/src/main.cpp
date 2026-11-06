#include "config.hpp"
#include "http.hpp"
#include "preset_routes.hpp"
#include "preset_store.hpp"
#include "redis_client.hpp"

#include <crow.h>
#include <sw/redis++/redis++.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

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

int main()
{
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
    PresetStore presets(*redis);

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Info);

    CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::Get)([&redis]()
    {
        return health(*redis);
    });

    addPresetRoutes(app, presets);

    CROW_LOG_INFO << "Redis at " << config.redisHost << ':' << config.redisPort;

    // Listens on all network interfaces, as Docker and Render need.
    // Crow stops cleanly on Ctrl+C and on the SIGTERM sent by "docker compose down".
    app.bindaddr("0.0.0.0").port(static_cast<std::uint16_t>(config.port)).multithreaded().run();
    return 0;
}

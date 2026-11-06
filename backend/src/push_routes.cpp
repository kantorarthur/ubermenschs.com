#include "push_routes.hpp"

#include "route_helpers.hpp"
#include "webpush_crypto.hpp"

#include <string>

void addPushRoutes(crow::SimpleApp& app, PushStore& subscriptions, VapidKeyStore& keys,
                   ReminderScheduler& scheduler, SessionStore& sessions, RateLimiter& limiter)
{
    CROW_ROUTE(app, "/api/push/public-key")
        .methods(crow::HTTPMethod::Get)
        ([&keys]()
    {
        return handle([&]()
        {
            crow::json::wvalue body;
            body["publicKey"] = base64UrlEncode(keys.keys().publicKey);
            return jsonResponse(200, body);
        });
    });

    CROW_ROUTE(app, "/api/push/subscriptions")
        .methods(crow::HTTPMethod::Post)
        ([&subscriptions, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);
            const crow::json::rvalue body = jsonObjectBody(request);
            // The shape of PushSubscription.toJSON() in the browser.
            if(!body.has("keys") || body["keys"].t() != crow::json::type::Object)
                throw ValidationError("The field \"keys\" must be an object.");

            const std::string id = subscriptions.save(userId, stringField(body, "endpoint"),
                                                      stringField(body["keys"], "p256dh"),
                                                      stringField(body["keys"], "auth"));
            crow::json::wvalue answer;
            answer["id"] = id;
            return jsonResponse(201, answer);
        });
    });

    CROW_ROUTE(app, "/api/push/unsubscribe")
        .methods(crow::HTTPMethod::Post)
        ([&subscriptions, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);
            const crow::json::rvalue body = jsonObjectBody(request);
            subscriptions.remove(userId, stringField(body, "endpoint"));
            return noContent();
        });
    });

    CROW_ROUTE(app, "/api/push/test")
        .methods(crow::HTTPMethod::Post)
        ([&subscriptions, &scheduler, &sessions, &limiter](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);
            limiter.hit(rateLimits::pushTestPerUser, userId);

            const int devices = static_cast<int>(subscriptions.list(userId).size());
            const int sent = scheduler.sendToUser(userId, Notification{"ubermenschs.com",
                "Notifications work. Your reminders will look like this.", "test", "/reminders"});

            crow::json::wvalue answer;
            answer["sent"] = sent;
            answer["devices"] = devices;
            return jsonResponse(200, answer);
        });
    });
}

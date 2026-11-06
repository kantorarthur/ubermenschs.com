#include "reminder_routes.hpp"

#include "route_helpers.hpp"

#include <string>
#include <utility>

namespace
{
    crow::json::wvalue toJson(const Reminder& reminder)
    {
        crow::json::wvalue json;
        json["id"] = reminder.id;
        json["time"] = reminder.time;
        json["message"] = reminder.message;
        json["enabled"] = reminder.enabled;
        json["timezone"] = reminder.timezone;
        return json;
    }
}

void addReminderRoutes(crow::SimpleApp& app, ReminderStore& store, SessionStore& sessions)
{
    CROW_ROUTE(app, "/api/reminders")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
        ([&store, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);

            if(request.method == crow::HTTPMethod::Get)
            {
                crow::json::wvalue::list reminders;
                for(const Reminder& reminder : store.list(userId))
                    reminders.push_back(toJson(reminder));
                return jsonResponse(200, crow::json::wvalue(std::move(reminders)));
            }

            const crow::json::rvalue body = jsonObjectBody(request);
            const Reminder reminder = store.create(userId, stringField(body, "time"), stringField(body, "message"),
                                                   stringField(body, "timezone"));
            return jsonResponse(201, toJson(reminder));
        });
    });

    CROW_ROUTE(app, "/api/reminders/<string>")
        .methods(crow::HTTPMethod::Patch, crow::HTTPMethod::Delete)
        ([&store, &sessions](const crow::request& request, std::string reminderId)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);

            if(request.method == crow::HTTPMethod::Patch)
            {
                const crow::json::rvalue body = jsonObjectBody(request);
                ReminderChanges changes;
                changes.time = optionalStringField(body, "time");
                changes.message = optionalStringField(body, "message");
                changes.enabled = optionalBoolField(body, "enabled");
                changes.timezone = optionalStringField(body, "timezone");
                return jsonResponse(200, toJson(store.update(userId, reminderId, changes)));
            }

            store.remove(userId, reminderId);
            return noContent();
        });
    });
}

#include "event_routes.hpp"

#include "route_helpers.hpp"

#include <string>
#include <utility>
#include <vector>

namespace
{
    crow::json::wvalue toJson(const CalendarEvent& event)
    {
        crow::json::wvalue json;
        json["id"] = event.id;
        json["title"] = event.title;
        json["start"] = event.start;
        json["end"] = event.end;
        json["allDay"] = event.allDay;
        json["description"] = event.description;
        return json;
    }

    EventInput eventFromBody(const crow::request& request)
    {
        const crow::json::rvalue body = jsonObjectBody(request);
        EventInput input;
        input.title = stringField(body, "title");
        input.start = stringField(body, "start");
        input.end = stringField(body, "end");
        input.allDay = boolField(body, "allDay");
        input.description = stringField(body, "description");
        return input;
    }

    // A query parameter such as ?from=2026-11-01, or "" if it is missing.
    std::string queryParameter(const crow::request& request, const std::string& name)
    {
        const char* value = request.url_params.get(name);
        return value != nullptr ? value : "";
    }
}

void addEventRoutes(crow::SimpleApp& app, EventStore& store, SessionStore& sessions)
{
    CROW_ROUTE(app, "/api/events")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
        ([&store, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);

            if(request.method == crow::HTTPMethod::Get)
            {
                const std::vector<CalendarEvent> found = store.list(userId, queryParameter(request, "from"),
                                                                    queryParameter(request, "to"));
                crow::json::wvalue::list events;
                for(const CalendarEvent& event : found)
                    events.push_back(toJson(event));
                return jsonResponse(200, crow::json::wvalue(std::move(events)));
            }

            const CalendarEvent event = store.create(userId, eventFromBody(request));
            return jsonResponse(201, toJson(event));
        });
    });

    CROW_ROUTE(app, "/api/events/<string>")
        .methods(crow::HTTPMethod::Patch, crow::HTTPMethod::Delete)
        ([&store, &sessions](const crow::request& request, std::string eventId)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);

            if(request.method == crow::HTTPMethod::Patch)
            {
                const CalendarEvent event = store.update(userId, eventId, eventFromBody(request));
                return jsonResponse(200, toJson(event));
            }

            store.remove(userId, eventId);
            return noContent();
        });
    });
}

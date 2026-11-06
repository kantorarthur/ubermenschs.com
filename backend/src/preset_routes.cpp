#include "preset_routes.hpp"

#include "route_helpers.hpp"

#include <optional>
#include <string>
#include <utility>

namespace
{
    crow::json::wvalue toJson(const Quote& quote)
    {
        crow::json::wvalue json;
        json["id"] = quote.id;
        json["text"] = quote.text;
        json["author"] = quote.author;
        return json;
    }

    crow::json::wvalue toJson(const Preset& preset)
    {
        crow::json::wvalue::list quotes;
        for(const Quote& quote : preset.quotes)
            quotes.push_back(toJson(quote));

        crow::json::wvalue json;
        json["id"] = preset.id;
        json["name"] = preset.name;
        json["quotes"] = std::move(quotes);
        return json;
    }
}

void addPresetRoutes(WebApp& app, PresetStore& store, SessionStore& sessions)
{
    CROW_ROUTE(app, "/api/presets")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
        ([&store, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);

            if(request.method == crow::HTTPMethod::Get)
            {
                crow::json::wvalue::list presets;
                for(const Preset& preset : store.list(userId))
                    presets.push_back(toJson(preset));
                return jsonResponse(200, crow::json::wvalue(std::move(presets)));
            }

            const crow::json::rvalue body = jsonObjectBody(request);
            const Preset preset = store.create(userId, stringField(body, "name"));
            return jsonResponse(201, toJson(preset));
        });
    });

    CROW_ROUTE(app, "/api/presets/<string>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Patch, crow::HTTPMethod::Delete)
        ([&store, &sessions](const crow::request& request, std::string presetId)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);

            if(request.method == crow::HTTPMethod::Get)
            {
                const std::optional<Preset> preset = store.find(userId, presetId);
                if(!preset)
                    throw NotFoundError("This preset does not exist.");
                return jsonResponse(200, toJson(*preset));
            }

            if(request.method == crow::HTTPMethod::Patch)
            {
                const crow::json::rvalue body = jsonObjectBody(request);
                const Preset preset = store.rename(userId, presetId, stringField(body, "name"));
                return jsonResponse(200, toJson(preset));
            }

            store.remove(userId, presetId);
            return noContent();
        });
    });

    CROW_ROUTE(app, "/api/presets/<string>/quotes")
        .methods(crow::HTTPMethod::Post)
        ([&store, &sessions](const crow::request& request, std::string presetId)
    {
        return handle([&]()
        {
            const crow::json::rvalue body = jsonObjectBody(request);
            const Quote quote = store.addQuote(requireUserId(request, sessions), presetId,
                                               stringField(body, "text"), stringField(body, "author"));
            return jsonResponse(201, toJson(quote));
        });
    });

    CROW_ROUTE(app, "/api/presets/<string>/quotes/<string>")
        .methods(crow::HTTPMethod::Delete)
        ([&store, &sessions](const crow::request& request, std::string presetId, std::string quoteId)
    {
        return handle([&]()
        {
            store.removeQuote(requireUserId(request, sessions), presetId, quoteId);
            return noContent();
        });
    });

    CROW_ROUTE(app, "/api/settings")
        .methods(crow::HTTPMethod::Get)
        ([&store, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            // Visitors always see Standard quotes in the header.
            const std::optional<std::string> userId = sessionUserId(request, sessions);
            crow::json::wvalue body;
            body["activePreset"] = userId ? store.activePreset(*userId) : STANDARD_PRESET_ID;
            return jsonResponse(200, body);
        });
    });

    CROW_ROUTE(app, "/api/settings/active-preset")
        .methods(crow::HTTPMethod::Put)
        ([&store, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const crow::json::rvalue body = jsonObjectBody(request);
            store.setActivePreset(requireUserId(request, sessions), stringField(body, "presetId"));
            return noContent();
        });
    });
}

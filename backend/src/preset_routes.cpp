#include "preset_routes.hpp"

#include "errors.hpp"
#include "http.hpp"

#include <sw/redis++/redis++.h>

#include <string>
#include <utility>

namespace
{
    // TEMPORARY: until login exists (step 5), every request belongs to this test user.
    std::string currentUserId(const crow::request&)
    {
        return "1";
    }

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

    // Runs a route and turns our exceptions into the right HTTP answer,
    // so each route only describes the successful case.
    template <typename Handler>
    crow::response handle(Handler&& handler)
    {
        try
        {
            return handler();
        }
        catch(const ValidationError& error)
        {
            return errorResponse(400, error.what());
        }
        catch(const NotFoundError& error)
        {
            return errorResponse(404, error.what());
        }
        catch(const sw::redis::Error& error)
        {
            CROW_LOG_ERROR << "Redis error: " << error.what();
            return errorResponse(503, "The database is not available right now. Try again in a moment.");
        }
    }
}

void addPresetRoutes(crow::SimpleApp& app, PresetStore& store)
{
    CROW_ROUTE(app, "/api/presets")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
        ([&store](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = currentUserId(request);

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
        ([&store](const crow::request& request, std::string presetId)
    {
        return handle([&]()
        {
            const std::string userId = currentUserId(request);

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
        ([&store](const crow::request& request, std::string presetId)
    {
        return handle([&]()
        {
            const crow::json::rvalue body = jsonObjectBody(request);
            const Quote quote = store.addQuote(currentUserId(request), presetId,
                                               stringField(body, "text"), stringField(body, "author"));
            return jsonResponse(201, toJson(quote));
        });
    });

    CROW_ROUTE(app, "/api/presets/<string>/quotes/<string>")
        .methods(crow::HTTPMethod::Delete)
        ([&store](const crow::request& request, std::string presetId, std::string quoteId)
    {
        return handle([&]()
        {
            store.removeQuote(currentUserId(request), presetId, quoteId);
            return noContent();
        });
    });

    CROW_ROUTE(app, "/api/settings")
        .methods(crow::HTTPMethod::Get)
        ([&store](const crow::request& request)
    {
        return handle([&]()
        {
            crow::json::wvalue body;
            body["activePreset"] = store.activePreset(currentUserId(request));
            return jsonResponse(200, body);
        });
    });

    CROW_ROUTE(app, "/api/settings/active-preset")
        .methods(crow::HTTPMethod::Put)
        ([&store](const crow::request& request)
    {
        return handle([&]()
        {
            const crow::json::rvalue body = jsonObjectBody(request);
            store.setActivePreset(currentUserId(request), stringField(body, "presetId"));
            return noContent();
        });
    });
}

#include "study_routes.hpp"

#include "route_helpers.hpp"

#include <string>
#include <utility>

namespace
{
    crow::json::wvalue progressJson(StudyStore& store, const std::string& userId)
    {
        crow::json::wvalue::list read;
        for(const std::string& bookId : store.readBooks(userId))
            read.push_back(bookId);

        crow::json::wvalue json;
        json["read"] = std::move(read);
        return json;
    }
}

void addStudyRoutes(WebApp& app, StudyStore& store, SessionStore& sessions)
{
    CROW_ROUTE(app, "/api/study")
        .methods(crow::HTTPMethod::Get)
        ([&store, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);
            return jsonResponse(200, progressJson(store, userId));
        });
    });

    CROW_ROUTE(app, "/api/study/books/<string>")
        .methods(crow::HTTPMethod::Put)
        ([&store, &sessions](const crow::request& request, std::string bookId)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);
            const crow::json::rvalue body = jsonObjectBody(request);
            store.setRead(userId, bookId, boolField(body, "read"));
            return jsonResponse(200, progressJson(store, userId));
        });
    });
}

#include "web_app.hpp"

#include <algorithm>

namespace
{
    bool isAllowed(const std::vector<std::string>& allowedOrigins, const std::string& origin)
    {
        return !origin.empty() && std::find(allowedOrigins.begin(), allowedOrigins.end(), origin) != allowedOrigins.end();
    }

    void addCorsHeaders(crow::response& response, const std::string& origin)
    {
        // The exact origin (never "*"), because the answer may carry the login cookie.
        response.set_header("Access-Control-Allow-Origin", origin);
        response.set_header("Access-Control-Allow-Credentials", "true");
        // Caches must keep the answers for different origins apart.
        response.set_header("Vary", "Origin");
    }

    // The browser's question before a POST/PATCH/PUT/DELETE with JSON: "may I send this?"
    bool isPreflight(const crow::request& request)
    {
        return request.method == crow::HTTPMethod::Options
            && !request.get_header_value("Access-Control-Request-Method").empty();
    }

    void answerPreflight(crow::response& response, const std::string& origin)
    {
        response.code = 204;
        response.body.clear();
        addCorsHeaders(response, origin);
        response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE");
        response.set_header("Access-Control-Allow-Headers", "Content-Type");
        // The browser may remember the answer for 10 minutes instead of asking before every request.
        response.set_header("Access-Control-Max-Age", "600");
    }
}

void Cors::before_handle(crow::request& request, crow::response& response, context& /*ctx*/)
{
    const std::string origin = request.get_header_value("Origin");
    if(isPreflight(request) && isAllowed(allowedOrigins, origin))
    {
        answerPreflight(response, origin);
        response.end();
    }
}

void Cors::after_handle(crow::request& request, crow::response& response, context& /*ctx*/)
{
    const std::string origin = request.get_header_value("Origin");
    if(!isAllowed(allowedOrigins, origin))
        return;

    // Crow may answer OPTIONS itself before before_handle runs; make that answer a proper preflight too.
    if(isPreflight(request))
        answerPreflight(response, origin);
    else
        addCorsHeaders(response, origin);
}

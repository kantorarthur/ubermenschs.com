#include "http.hpp"

#include "errors.hpp"

crow::response jsonResponse(int status, const crow::json::wvalue& body)
{
    crow::response response(status);
    response.set_header("Content-Type", "application/json");
    response.body = body.dump();
    return response;
}

crow::response errorResponse(int status, const std::string& message)
{
    crow::json::wvalue body;
    body["error"] = message;
    return jsonResponse(status, body);
}

crow::response noContent()
{
    return crow::response(204);
}

crow::json::rvalue jsonObjectBody(const crow::request& request)
{
    crow::json::rvalue body = crow::json::load(request.body);
    if(!body || body.t() != crow::json::type::Object)
        throw ValidationError("The request body must be a JSON object.");
    return body;
}

std::string stringField(const crow::json::rvalue& body, const std::string& name)
{
    if(!body.has(name) || body[name].t() != crow::json::type::String)
        throw ValidationError("The field \"" + name + "\" must be text.");
    return std::string(body[name].s());
}

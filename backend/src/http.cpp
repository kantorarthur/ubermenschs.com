#include "http.hpp"

#include "errors.hpp"
#include "text.hpp"

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

bool boolField(const crow::json::rvalue& body, const std::string& name)
{
    const std::optional<bool> value = optionalBoolField(body, name);
    if(!value)
        throw ValidationError("The field \"" + name + "\" must be true or false.");
    return *value;
}

std::optional<std::string> optionalStringField(const crow::json::rvalue& body, const std::string& name)
{
    if(!body.has(name))
        return std::nullopt;
    return stringField(body, name);
}

std::optional<bool> optionalBoolField(const crow::json::rvalue& body, const std::string& name)
{
    if(!body.has(name))
        return std::nullopt;

    const crow::json::type type = body[name].t();
    if(type != crow::json::type::True && type != crow::json::type::False)
        throw ValidationError("The field \"" + name + "\" must be true or false.");
    return type == crow::json::type::True;
}

std::string cookieValue(const crow::request& request, const std::string& name)
{
    // The browser sends all cookies in one header: "Cookie: a=1; session=abc; b=2".
    const std::string& header = request.get_header_value("Cookie");

    std::size_t start = 0;
    while(start < header.size())
    {
        std::size_t end = header.find(';', start);
        if(end == std::string::npos)
            end = header.size();

        std::size_t first = start;
        while(first < end && header[first] == ' ')
            ++first;

        const std::size_t equals = header.find('=', first);
        if(equals != std::string::npos && equals < end && header.compare(first, equals - first, name) == 0)
            return header.substr(equals + 1, end - equals - 1);

        start = end + 1;
    }
    return "";
}

std::string clientIp(const crow::request& request, bool trustProxy)
{
    if(trustProxy)
    {
        // "X-Forwarded-For: a, b, c". Each proxy adds the address it got the request from at the end,
        // and the visitor can write anything at the start. So the last address is the one our own
        // proxy (Render) saw: the only one that cannot be faked, as long as nothing else stands in between.
        const std::string& forwarded = request.get_header_value("X-Forwarded-For");
        const std::size_t comma = forwarded.rfind(',');
        const std::string last = trim(comma == std::string::npos ? forwarded : forwarded.substr(comma + 1));
        if(!last.empty())
            return last;
    }
    return request.remote_ip_address;
}

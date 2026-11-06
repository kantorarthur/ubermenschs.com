#pragma once

#include <crow.h>

#include <optional>
#include <string>

// A response whose body is the given JSON.
crow::response jsonResponse(int status, const crow::json::wvalue& body);

// {"error": "<message>"} with the given status, the shape the frontend expects for errors.
crow::response errorResponse(int status, const std::string& message);

// 204 No Content: success, nothing to send back.
crow::response noContent();

// The request body as a JSON object. Throws ValidationError if it is not one.
crow::json::rvalue jsonObjectBody(const crow::request& request);

// A text field of a JSON object. Throws ValidationError if it is missing or not text.
std::string stringField(const crow::json::rvalue& body, const std::string& name);

// A true/false field of a JSON object. Throws ValidationError if it is missing or not true/false.
bool boolField(const crow::json::rvalue& body, const std::string& name);

// For changes where every field may be left out (PATCH): std::nullopt if the field is missing,
// ValidationError if it is there with the wrong type.
std::optional<std::string> optionalStringField(const crow::json::rvalue& body, const std::string& name);
std::optional<bool> optionalBoolField(const crow::json::rvalue& body, const std::string& name);

// The IP address of the visitor. Behind a proxy (Render), every request comes from the proxy,
// so with trustProxy the address is taken from the X-Forwarded-For header the proxy adds.
// Only turn trustProxy on behind a proxy: otherwise anyone could send a fake header.
std::string clientIp(const crow::request& request, bool trustProxy);

// The value of one cookie sent by the browser, or "" if it is not there.
std::string cookieValue(const crow::request& request, const std::string& name);

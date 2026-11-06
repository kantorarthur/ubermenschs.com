#pragma once

#include <crow.h>

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

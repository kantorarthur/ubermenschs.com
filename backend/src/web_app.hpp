#pragma once

#include <crow.h>

#include <string>
#include <vector>

// CORS (Cross-Origin Resource Sharing). In production the site is https://ubermenschs.com and the API
// https://api.ubermenschs.com: two origins, so the browser only lets the site read the API's answers
// (and send the login cookie) if the API says that origin is allowed.
//
// Only the origins in allowedOrigins get the CORS headers; for any other site the browser blocks
// the answer. Locally the list is empty: Vite forwards /api, so the browser sees one origin.
struct Cors
{
    struct context
    {
    };

    std::vector<std::string> allowedOrigins;

    void before_handle(crow::request& request, crow::response& response, context& ctx);
    void after_handle(crow::request& request, crow::response& response, context& ctx);
};

// The Crow application type of the server: Crow with the CORS middleware.
using WebApp = crow::App<Cors>;

#pragma once

#include "errors.hpp"
#include "http.hpp"
#include "user_store.hpp"

#include <crow.h>
#include <sw/redis++/redis++.h>

#include <optional>
#include <string>

// The name of the login cookie.
inline const std::string SESSION_COOKIE = "session";

// The id of the logged-in user, from the session cookie, or std::nullopt for a visitor.
inline std::optional<std::string> sessionUserId(const crow::request& request, SessionStore& sessions)
{
    const std::string token = cookieValue(request, SESSION_COOKIE);
    if(token.empty())
        return std::nullopt;
    return sessions.userIdFor(token);
}

// Like sessionUserId, but throws UnauthorizedError (HTTP 401) for a visitor.
inline std::string requireUserId(const crow::request& request, SessionStore& sessions)
{
    const std::optional<std::string> userId = sessionUserId(request, sessions);
    if(!userId)
        throw UnauthorizedError("Log in to do this.");
    return *userId;
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
    catch(const UnauthorizedError& error)
    {
        return errorResponse(401, error.what());
    }
    catch(const NotFoundError& error)
    {
        return errorResponse(404, error.what());
    }
    catch(const ConflictError& error)
    {
        return errorResponse(409, error.what());
    }
    catch(const TooManyRequestsError& error)
    {
        crow::response response = errorResponse(429, error.what());
        response.set_header("Retry-After", std::to_string(error.retryAfterSeconds()));
        return response;
    }
    catch(const sw::redis::Error& error)
    {
        CROW_LOG_ERROR << "Redis error: " << error.what();
        return errorResponse(503, "The database is not available right now. Try again in a moment.");
    }
}

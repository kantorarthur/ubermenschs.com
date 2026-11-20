#include "auth_routes.hpp"

#include "rate_limiter.hpp"
#include "route_helpers.hpp"
#include "text.hpp"

#include <chrono>
#include <optional>
#include <string>

namespace
{
    crow::json::wvalue toJson(const User& user)
    {
        crow::json::wvalue json;
        json["id"] = user.id;
        json["email"] = user.email;
        json["username"] = user.username;
        json["createdAt"] = user.createdAt;
        return json;
    }

    // The Set-Cookie header for the login cookie.
    //   HttpOnly  JavaScript cannot read it, so a script injected into the page cannot steal it.
    //   SameSite  other websites cannot make the browser send it with their requests.
    //   Secure    sent only over HTTPS (on in production).
    std::string sessionCookie(const std::string& token, long long maxAgeSeconds, const Config& config)
    {
        std::string cookie = SESSION_COOKIE + "=" + token
            + "; Path=/; HttpOnly; SameSite=Lax; Max-Age=" + std::to_string(maxAgeSeconds);
        if(config.cookieSecure)
            cookie += "; Secure";
        if(!config.cookieDomain.empty())
            cookie += "; Domain=" + config.cookieDomain;
        return cookie;
    }

    crow::response loggedIn(int status, const User& user, const std::string& token, const Config& config)
    {
        const long long maxAge = std::chrono::duration_cast<std::chrono::seconds>(sessionLifetime).count();
        crow::response response = jsonResponse(status, toJson(user));
        response.add_header("Set-Cookie", sessionCookie(token, maxAge, config));
        return response;
    }
}

void addAuthRoutes(WebApp& app, UserStore& users, SessionStore& sessions, RateLimiter& limiter,
                   const Config& config)
{
    CROW_ROUTE(app, "/api/auth/register")
        .methods(crow::HTTPMethod::Post)
        ([&users, &sessions, &limiter, &config](const crow::request& request)
    {
        return handle([&]()
        {
            limiter.hit(rateLimits::registerPerIp, clientIp(request, config.trustProxy));

            const crow::json::rvalue body = jsonObjectBody(request);
            const User user = users.registerUser(stringField(body, "email"), stringField(body, "username"),
                                                 stringField(body, "password"));
            return loggedIn(201, user, sessions.create(user.id), config);
        });
    });

    CROW_ROUTE(app, "/api/auth/login")
        .methods(crow::HTTPMethod::Post)
        ([&users, &sessions, &limiter, &config](const crow::request& request)
    {
        return handle([&]()
        {
            // Every attempt from this IP address counts, right or wrong.
            limiter.hit(rateLimits::loginPerIp, clientIp(request, config.trustProxy));

            const crow::json::rvalue body = jsonObjectBody(request);
            const std::string email = normalizeEmail(stringField(body, "email"));

            // An account with too many wrong passwords is locked for a while. This is checked
            // before the password, so a locked account answers fast and never runs Argon2.
            limiter.check(rateLimits::loginFailuresPerEmail, email);

            const std::optional<User> user = users.authenticate(email, stringField(body, "password"));
            // The same message whether the email or the password is wrong,
            // so the form does not reveal which emails have accounts.
            if(!user)
            {
                limiter.hit(rateLimits::loginFailuresPerEmail, email);
                throw UnauthorizedError("Wrong email or password.");
            }

            limiter.reset(rateLimits::loginFailuresPerEmail, email);
            return loggedIn(200, *user, sessions.create(user->id), config);
        });
    });

    CROW_ROUTE(app, "/api/auth/logout")
        .methods(crow::HTTPMethod::Post)
        ([&sessions, &config](const crow::request& request)
    {
        return handle([&]()
        {
            sessions.remove(cookieValue(request, SESSION_COOKIE));

            // An expired, empty cookie makes the browser delete it.
            crow::response response = noContent();
            response.add_header("Set-Cookie", sessionCookie("", 0, config));
            return response;
        });
    });

    CROW_ROUTE(app, "/api/auth/password")
        .methods(crow::HTTPMethod::Post)
        ([&users, &sessions, &limiter](const crow::request& request)
    {
        return handle([&]()
        {
            const std::string userId = requireUserId(request, sessions);
            const crow::json::rvalue body = jsonObjectBody(request);

            // Checked before the password, like the login: a locked account never runs Argon2.
            limiter.check(rateLimits::passwordChangeFailuresPerUser, userId);
            try
            {
                users.changePassword(userId, stringField(body, "currentPassword"), stringField(body, "newPassword"));
            }
            catch(const UnauthorizedError&)
            {
                limiter.hit(rateLimits::passwordChangeFailuresPerUser, userId);
                // 400, not 401: the user is still logged in, only the typed password is wrong.
                throw ValidationError("The current password is wrong.");
            }
            limiter.reset(rateLimits::passwordChangeFailuresPerUser, userId);

            // Someone who knew the old password is logged out everywhere else; this device stays logged in.
            sessions.removeAllExcept(userId, cookieValue(request, SESSION_COOKIE));
            return noContent();
        });
    });

    CROW_ROUTE(app, "/api/auth/me")
        .methods(crow::HTTPMethod::Get)
        ([&users, &sessions](const crow::request& request)
    {
        return handle([&]()
        {
            const std::optional<User> user = users.findById(requireUserId(request, sessions));
            if(!user)
                throw UnauthorizedError("Log in to do this.");
            return jsonResponse(200, toJson(*user));
        });
    });
}

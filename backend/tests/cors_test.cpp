#include "web_app.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{
    const std::string SITE = "https://ubermenschs.com";

    crow::request requestFrom(const std::string& origin, crow::HTTPMethod method)
    {
        crow::request request;
        request.method = method;
        if(!origin.empty())
            request.add_header("Origin", origin);
        return request;
    }

    Cors corsFor(const std::string& origin)
    {
        Cors cors;
        cors.allowedOrigins = {origin};
        return cors;
    }
}

TEST(Cors, TheSiteGetsTheHeadersWithItsExactOrigin)
{
    Cors cors = corsFor(SITE);
    Cors::context ctx;
    crow::request request = requestFrom(SITE, crow::HTTPMethod::Post);
    crow::response response(201);

    cors.after_handle(request, response, ctx);
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), SITE);
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Credentials"), "true");
    EXPECT_EQ(response.get_header_value("Vary"), "Origin");
    EXPECT_EQ(response.code, 201);
}

TEST(Cors, OtherSitesGetNothing)
{
    Cors cors = corsFor(SITE);
    Cors::context ctx;
    for(const std::string& origin : {std::string("https://evil.example"), std::string("https://ubermenschs.com.evil.example"),
                                     std::string("http://ubermenschs.com"), std::string("")})
    {
        crow::request request = requestFrom(origin, crow::HTTPMethod::Post);
        crow::response response(200);
        cors.after_handle(request, response, ctx);
        EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), "") << origin;
        EXPECT_EQ(response.get_header_value("Access-Control-Allow-Credentials"), "") << origin;
    }
}

TEST(Cors, PreflightIsAnsweredBeforeTheRoute)
{
    Cors cors = corsFor(SITE);
    Cors::context ctx;
    crow::request request = requestFrom(SITE, crow::HTTPMethod::Options);
    request.add_header("Access-Control-Request-Method", "PATCH");
    crow::response response;

    cors.before_handle(request, response, ctx);
    EXPECT_TRUE(response.is_completed());
    EXPECT_EQ(response.code, 204);
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), SITE);
    EXPECT_NE(response.get_header_value("Access-Control-Allow-Methods").find("PATCH"), std::string::npos);
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Headers"), "Content-Type");
}

TEST(Cors, PreflightFromAnotherSiteIsNotAnswered)
{
    Cors cors = corsFor(SITE);
    Cors::context ctx;
    crow::request request = requestFrom("https://evil.example", crow::HTTPMethod::Options);
    request.add_header("Access-Control-Request-Method", "DELETE");
    crow::response response;

    cors.before_handle(request, response, ctx);
    EXPECT_FALSE(response.is_completed());
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), "");
}

TEST(Cors, NothingIsAllowedByDefault)
{
    Cors cors;
    Cors::context ctx;
    crow::request request = requestFrom(SITE, crow::HTTPMethod::Get);
    crow::response response(200);
    cors.after_handle(request, response, ctx);
    EXPECT_EQ(response.get_header_value("Access-Control-Allow-Origin"), "");
}

#include "errors.hpp"
#include "http.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace
{
    crow::request requestWithHeader(const std::string& name, const std::string& value)
    {
        crow::request request;
        request.add_header(name, value);
        return request;
    }
}

TEST(CookieValue, FindsOneCookieAmongSeveral)
{
    const crow::request request = requestWithHeader("Cookie", "theme=dark; session=abc123; lang=ro");
    EXPECT_EQ(cookieValue(request, "session"), "abc123");
    EXPECT_EQ(cookieValue(request, "theme"), "dark");
    EXPECT_EQ(cookieValue(request, "lang"), "ro");
}

TEST(CookieValue, MissingCookieIsEmpty)
{
    EXPECT_EQ(cookieValue(requestWithHeader("Cookie", "theme=dark"), "session"), "");
    EXPECT_EQ(cookieValue(crow::request(), "session"), "");
}

TEST(CookieValue, NameMustMatchExactly)
{
    // "xsession" and "session_old" are other cookies, not "session".
    const crow::request request = requestWithHeader("Cookie", "xsession=1; session_old=2");
    EXPECT_EQ(cookieValue(request, "session"), "");
}

TEST(ClientIp, WithoutTrustProxyUsesTheConnectionAddress)
{
    crow::request request = requestWithHeader("X-Forwarded-For", "9.9.9.9");
    request.remote_ip_address = "10.0.0.5";

    // A visitor could write any X-Forwarded-For; without a proxy it must be ignored.
    EXPECT_EQ(clientIp(request, false), "10.0.0.5");
}

TEST(ClientIp, WithTrustProxyUsesTheAddressThatOurProxyAdded)
{
    crow::request request = requestWithHeader("X-Forwarded-For", " 1.2.3.4 , 10.0.0.1, 10.0.0.2");
    request.remote_ip_address = "10.0.0.2";
    // Render adds the address it saw at the end; anything before it came from the visitor.
    EXPECT_EQ(clientIp(request, true), "10.0.0.2");

    const crow::request faked = requestWithHeader("X-Forwarded-For", "1.1.1.1, 203.0.113.7");
    EXPECT_EQ(clientIp(faked, true), "203.0.113.7");
    const crow::request single = requestWithHeader("X-Forwarded-For", "203.0.113.7");
    EXPECT_EQ(clientIp(single, true), "203.0.113.7");
}

TEST(ClientIp, WithTrustProxyButNoHeaderFallsBackToTheConnection)
{
    crow::request request;
    request.remote_ip_address = "10.0.0.5";
    EXPECT_EQ(clientIp(request, true), "10.0.0.5");
}

TEST(Http, BoolAndOptionalFields)
{
    const crow::json::rvalue body = crow::json::load(R"({"on": true, "off": false, "name": "Ana", "count": 1})");

    EXPECT_TRUE(boolField(body, "on"));
    EXPECT_FALSE(boolField(body, "off"));
    EXPECT_THROW(boolField(body, "name"), ValidationError);
    EXPECT_THROW(boolField(body, "missing"), ValidationError);

    EXPECT_EQ(optionalStringField(body, "name"), std::optional<std::string>("Ana"));
    EXPECT_FALSE(optionalStringField(body, "missing"));
    EXPECT_THROW(optionalStringField(body, "count"), ValidationError);

    EXPECT_EQ(optionalBoolField(body, "off"), std::optional<bool>(false));
    EXPECT_FALSE(optionalBoolField(body, "missing"));
    EXPECT_THROW(optionalBoolField(body, "count"), ValidationError);
}

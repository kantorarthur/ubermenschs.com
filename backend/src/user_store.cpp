#include "user_store.hpp"

#include "crypto.hpp"
#include "errors.hpp"
#include "redis_client.hpp"
#include "text.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iterator>
#include <string>
#include <unordered_map>

namespace
{
    std::string userKey(const std::string& userId)
    {
        return "user:" + userId;
    }

    std::string emailKey(const std::string& email)
    {
        return "user:by-email:" + email;
    }

    std::string sessionKey(const std::string& token)
    {
        return "session:" + fingerprint(token);
    }

    // A simple shape check: something@something.something, without spaces.
    // The only real proof that an email works is sending a message to it.
    bool looksLikeEmail(const std::string& email)
    {
        const std::size_t at = email.find('@');
        if(at == std::string::npos || at == 0 || email.find('@', at + 1) != std::string::npos)
            return false;

        const std::size_t dot = email.find('.', at + 2);
        if(dot == std::string::npos || dot == email.size() - 1)
            return false;

        return std::none_of(email.begin(), email.end(), [](unsigned char c)
        {
            return std::isspace(c) != 0;
        });
    }

    std::string checkPassword(const std::string& password)
    {
        const std::size_t length = utf8Length(password);
        if(length < limits::passwordMin)
            throw ValidationError("The password needs at least " + std::to_string(limits::passwordMin) + " characters.");
        if(length > limits::passwordMax)
            throw ValidationError("The password can have at most " + std::to_string(limits::passwordMax) + " characters.");
        return password;
    }

    std::string secondsSinceEpoch()
    {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now).count());
    }
}

UserStore::UserStore(sw::redis::Redis& redis)
    : redis_(redis)
{
}

User UserStore::registerUser(const std::string& email, const std::string& username, const std::string& password)
{
    const std::string cleanEmail = normalizeEmail(email);
    if(cleanEmail.empty())
        throw ValidationError("Enter your email.");
    if(utf8Length(cleanEmail) > limits::emailLength || !looksLikeEmail(cleanEmail))
        throw ValidationError("This does not look like an email address.");

    const std::string cleanUsername = trim(username);
    const std::size_t usernameLength = utf8Length(cleanUsername);
    if(usernameLength < limits::usernameMin || usernameLength > limits::usernameMax)
    {
        throw ValidationError("The username needs " + std::to_string(limits::usernameMin) + " to "
                              + std::to_string(limits::usernameMax) + " characters.");
    }

    // Hash first: it is the slow part, and a failure here leaves nothing behind in Redis.
    const std::string passwordHash = hashPassword(checkPassword(password));
    const std::string userId = std::to_string(redis_.incr("next:user:id"));

    // SET ... NX succeeds only for the first account with this email, even if two people
    // register with it at the same moment.
    if(!redis_.setnx(emailKey(cleanEmail), userId))
    {
        // The email is taken, unless an earlier registration stopped halfway
        // (email claimed, account never written). Then the email can be reused.
        const auto existingId = redis_.get(emailKey(cleanEmail));
        if(existingId && redis_.exists(userKey(*existingId)) == 1)
            throw ConflictError("An account with this email already exists.");
        redis_.set(emailKey(cleanEmail), userId);
    }

    auto transaction = redis_.transaction();
    transaction.hset(userKey(userId), "email", cleanEmail)
               .hset(userKey(userId), "username", cleanUsername)
               .hset(userKey(userId), "passwordHash", passwordHash)
               .hset(userKey(userId), "createdAt", secondsSinceEpoch());
    transaction.exec();
    waitForReplica(transaction.redis());

    return User{userId, cleanEmail, cleanUsername};
}

std::optional<User> UserStore::authenticate(const std::string& email, const std::string& password)
{
    const auto userId = redis_.get(emailKey(normalizeEmail(email)));
    if(!userId)
        return std::nullopt;

    std::unordered_map<std::string, std::string> fields;
    redis_.hgetall(userKey(*userId), std::inserter(fields, fields.begin()));
    if(fields.empty() || !verifyPassword(fields["passwordHash"], password))
        return std::nullopt;

    return User{*userId, fields["email"], fields["username"]};
}

std::optional<User> UserStore::findById(const std::string& userId)
{
    std::unordered_map<std::string, std::string> fields;
    redis_.hgetall(userKey(userId), std::inserter(fields, fields.begin()));
    if(fields.empty())
        return std::nullopt;

    return User{userId, fields["email"], fields["username"]};
}

SessionStore::SessionStore(sw::redis::Redis& redis)
    : redis_(redis)
{
}

std::string SessionStore::create(const std::string& userId)
{
    const std::string token = randomToken();
    redis_.set(sessionKey(token), userId, std::chrono::duration_cast<std::chrono::milliseconds>(sessionLifetime));
    return token;
}

std::optional<std::string> SessionStore::userIdFor(const std::string& token)
{
    // Our tokens are exactly 64 hex characters; anything else cannot be a session.
    if(token.size() != 64)
        return std::nullopt;

    const auto userId = redis_.get(sessionKey(token));
    if(!userId)
        return std::nullopt;
    return *userId;
}

void SessionStore::remove(const std::string& token)
{
    if(token.size() == 64)
        redis_.del(sessionKey(token));
}

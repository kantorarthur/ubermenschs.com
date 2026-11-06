#pragma once

#include <sw/redis++/redis++.h>

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

struct User
{
    std::string id;
    std::string email;
    std::string username;
};

namespace limits
{
    inline constexpr std::size_t emailLength = 254;
    inline constexpr std::size_t usernameMin = 2;
    inline constexpr std::size_t usernameMax = 30;
    inline constexpr std::size_t passwordMin = 10;
    inline constexpr std::size_t passwordMax = 200;
}

// How long a login lasts without logging in again.
inline constexpr std::chrono::hours sessionLifetime{24 * 30};

// Accounts in Redis, with the keys from the plan:
//
//   next:user:id              counter for new user ids
//   user:{id}                 hash: email, username, passwordHash, createdAt
//   user:by-email:{email}     the user id for an email (set with NX, so emails stay unique)
class UserStore
{
public:
    explicit UserStore(sw::redis::Redis& redis);

    // Creates the account; throws ValidationError for bad data and ConflictError if the email is taken.
    User registerUser(const std::string& email, const std::string& username, const std::string& password);

    // The user if the email and password match, otherwise std::nullopt.
    std::optional<User> authenticate(const std::string& email, const std::string& password);

    std::optional<User> findById(const std::string& userId);

private:
    sw::redis::Redis& redis_;
};

// Logins, with the key from the plan:
//
//   session:{fingerprint of the token}   the user id; expires after sessionLifetime
class SessionStore
{
public:
    explicit SessionStore(sw::redis::Redis& redis);

    // Starts a session and returns the token for the cookie.
    std::string create(const std::string& userId);

    // The user id of a valid session, or std::nullopt.
    std::optional<std::string> userIdFor(const std::string& token);

    void remove(const std::string& token);

private:
    sw::redis::Redis& redis_;
};

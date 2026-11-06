#pragma once

#include <stdexcept>
#include <string>

// The request data is not acceptable (empty name, text too long, ...). Becomes HTTP 400.
class ValidationError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// The request needs a logged-in user, or the login data is wrong. Becomes HTTP 401.
class UnauthorizedError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// The thing asked for does not exist, or belongs to another user. Becomes HTTP 404.
class NotFoundError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// The request clashes with existing data, e.g. an email that already has an account. Becomes HTTP 409.
class ConflictError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// Too many requests in a short time (rate limit). Becomes HTTP 429 with a Retry-After header.
class TooManyRequestsError : public std::runtime_error
{
public:
    TooManyRequestsError(const std::string& message, long long retryAfterSeconds)
        : std::runtime_error(message)
        , retryAfterSeconds_(retryAfterSeconds)
    {
    }

    long long retryAfterSeconds() const
    {
        return retryAfterSeconds_;
    }

private:
    long long retryAfterSeconds_;
};

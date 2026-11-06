#pragma once

#include <stdexcept>

// The request data is not acceptable (empty name, text too long, ...). Becomes HTTP 400.
class ValidationError : public std::runtime_error
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

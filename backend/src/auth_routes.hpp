#pragma once

#include "account_deleter.hpp"
#include "config.hpp"
#include "rate_limiter.hpp"
#include "user_store.hpp"
#include "web_app.hpp"

#include <crow.h>

// Adds the account routes:
//
//   POST /api/auth/register   { email, username, password }   → 201 + the user, logged in
//   POST /api/auth/login      { email, password }             → 200 + the user, logged in
//   POST /api/auth/logout                                     → 204, logged out
//   GET  /api/auth/me                                         → 200 + the user, or 401
//   POST /api/auth/password   { currentPassword, newPassword } → 204; other devices are logged out
//   DELETE /api/auth/account  { password }                    → 204; the account and all its data are deleted
//
// "Logged in" means the answer sets the HttpOnly "session" cookie.
// Register and login are rate limited (see rateLimits): over the limit they answer 429.
void addAuthRoutes(WebApp& app, UserStore& users, SessionStore& sessions, AccountDeleter& accounts,
                   RateLimiter& limiter, const Config& config);

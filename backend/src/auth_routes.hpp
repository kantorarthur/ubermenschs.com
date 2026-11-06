#pragma once

#include "config.hpp"
#include "rate_limiter.hpp"
#include "user_store.hpp"

#include <crow.h>

// Adds the account routes:
//
//   POST /api/auth/register   { email, username, password }   → 201 + the user, logged in
//   POST /api/auth/login      { email, password }             → 200 + the user, logged in
//   POST /api/auth/logout                                     → 204, logged out
//   GET  /api/auth/me                                         → 200 + the user, or 401
//   POST /api/auth/password   { currentPassword, newPassword } → 204; other devices are logged out
//
// "Logged in" means the answer sets the HttpOnly "session" cookie.
// Register and login are rate limited (see rateLimits): over the limit they answer 429.
void addAuthRoutes(crow::SimpleApp& app, UserStore& users, SessionStore& sessions, RateLimiter& limiter,
                   const Config& config);

#pragma once

#include "reminder_store.hpp"
#include "user_store.hpp"

#include <crow.h>

// Adds the reminder routes (a user can have several reminders):
//
//   GET    /api/reminders        the user's reminders, sorted by time of day
//   POST   /api/reminders        { time, message, timezone }   → 201 + the reminder (it starts on)
//   PATCH  /api/reminders/:id    any of { time, message, enabled, timezone }   → the reminder
//   DELETE /api/reminders/:id    → 204
//
// All of them need a logged-in user (otherwise 401).
void addReminderRoutes(crow::SimpleApp& app, ReminderStore& store, SessionStore& sessions);

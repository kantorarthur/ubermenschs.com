#pragma once

#include "event_store.hpp"
#include "user_store.hpp"

#include <crow.h>

// Adds the calendar routes:
//
//   GET    /api/events?from=YYYY-MM-DD&to=YYYY-MM-DD   events touching those days, sorted by start
//   POST   /api/events        { title, start, end, allDay, description }   → 201 + the event
//   PATCH  /api/events/:id    { title, start, end, allDay, description }   → the event (all fields replaced)
//   DELETE /api/events/:id    → 204
//
// All of them need a logged-in user (otherwise 401).
void addEventRoutes(crow::SimpleApp& app, EventStore& store, SessionStore& sessions);

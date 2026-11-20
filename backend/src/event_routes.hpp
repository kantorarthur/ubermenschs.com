#pragma once

#include "event_store.hpp"
#include "user_store.hpp"
#include "web_app.hpp"

#include <crow.h>

// Adds the calendar routes:
//
//   GET    /api/events?from=YYYY-MM-DD&to=YYYY-MM-DD   events touching those days, sorted by start
//   POST   /api/events        { title, start, end, allDay, description, notify?, timezone? }   → 201 + the event
//   PATCH  /api/events/:id    the same fields   → the event (all fields replaced)
//
//   notify: minutes before the start to send a notification, any of 0, 10, 30, 60, 1440 (several allowed);
//   timezone: the browser's time zone, needed when notify is not empty.
//   DELETE /api/events/:id    → 204
//
// All of them need a logged-in user (otherwise 401).
void addEventRoutes(WebApp& app, EventStore& store, SessionStore& sessions);

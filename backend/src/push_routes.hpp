#pragma once

#include "push_sender.hpp"
#include "push_store.hpp"
#include "rate_limiter.hpp"
#include "reminder_scheduler.hpp"
#include "user_store.hpp"
#include "web_app.hpp"

#include <crow.h>

// Adds the push notification routes:
//
//   GET  /api/push/public-key       { publicKey }: the VAPID key the browser needs to subscribe
//   POST /api/push/subscriptions    { endpoint, keys: { p256dh, auth } }   → 201 { id }
//   POST /api/push/unsubscribe      { endpoint }   → 204
//   POST /api/push/test             → { sent, devices }: a test notification to every browser of the user
//
// All except the public key need a logged-in user (otherwise 401).
void addPushRoutes(WebApp& app, PushStore& subscriptions, VapidKeyStore& keys,
                   ReminderScheduler& scheduler, SessionStore& sessions, RateLimiter& limiter);

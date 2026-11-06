#pragma once

#include "preset_store.hpp"
#include "user_store.hpp"

#include <crow.h>

// Adds the quote preset routes:
//
//   GET    /api/presets                          the user's presets, with their quotes
//   POST   /api/presets                          { name }            → 201 + the preset
//   GET    /api/presets/:id                      one preset
//   PATCH  /api/presets/:id                      { name }            → the preset
//   DELETE /api/presets/:id                      → 204
//   POST   /api/presets/:id/quotes               { text, author }    → 201 + the quote
//   DELETE /api/presets/:id/quotes/:quoteId      → 204
//   GET    /api/settings                         { activePreset }
//   PUT    /api/settings/active-preset           { presetId }        → 204
//
// Everything except GET /api/settings needs a logged-in user (otherwise 401).
// Errors are {"error": "..."} with 400 (bad data), 401 (not logged in), 404 (not found) or 503 (Redis down).
void addPresetRoutes(crow::SimpleApp& app, PresetStore& store, SessionStore& sessions);

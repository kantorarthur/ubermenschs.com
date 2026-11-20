#pragma once

#include "study_store.hpp"
#include "user_store.hpp"
#include "web_app.hpp"

#include <crow.h>

// Adds the study plan routes:
//
//   GET /api/study                 { read: ["meditations", ...] }: the books marked as read
//   PUT /api/study/books/:bookId   { read: true | false }   → the same answer as GET
//
// Both need a logged-in user (otherwise 401).
void addStudyRoutes(WebApp& app, StudyStore& store, SessionStore& sessions);

#pragma once

#include <chrono>
#include <string>

// Checks for the date and time strings the frontend sends. They are local times
// (no time zone), exactly as the user typed them.

// "YYYY-MM-DD", a real day: "2026-02-30" is refused.
bool isDate(const std::string& text);

// "YYYY-MM-DDTHH:MM", a real day and a time from 00:00 to 23:59.
bool isLocalDateTime(const std::string& text);

// "HH:MM", from 00:00 to 23:59.
bool isTimeOfDay(const std::string& text);

// A time zone from the tz database, e.g. "Europe/Bucharest" or "UTC".
// Needs the tzdata package on Linux (installed in the Docker images).
bool isTimeZone(const std::string& name);

// Days from "from" to "to" (both "YYYY-MM-DD"); 0 for the same day, negative if "to" is earlier.
int daysBetween(const std::string& from, const std::string& to);

// A number that sorts like the moment it describes, used as the score in Redis:
// "2026-11-06T14:30" becomes 202611061430. Expects a string isLocalDateTime accepts.
long long dateTimeNumber(const std::string& dateTime);

// The next moment strictly after `after` when a daily reminder set for `time` ("HH:MM") in the time zone
// `timezone` goes off, in UTC. Summer and winter time are handled:
//   - a time that is skipped when the clocks go forward (02:30 on that night) goes off at the switch;
//   - a time that happens twice when the clocks go back goes off only the first time.
// Expects values isTimeOfDay and isTimeZone accept; throws std::runtime_error for an unknown time zone.
std::chrono::sys_seconds nextReminderTime(const std::string& time, const std::string& timezone,
                                          std::chrono::sys_seconds after);

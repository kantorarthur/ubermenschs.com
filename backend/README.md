# ubermenschs.com — backend

C++20 server built with [Crow](https://crowcpp.org) (HTTP), [redis-plus-plus](https://github.com/sewenew/redis-plus-plus) (Redis)
and [libsodium](https://libsodium.org) (password hashing).
Libraries come from vcpkg (`vcpkg.json`).

## Run it with Docker (recommended)

From the repository root (the folder that holds `compose.yaml`):

```
docker compose up --build
```

This starts three containers: the backend, the Redis primary and the Redis replica.
The first build downloads and compiles the libraries, so it takes several minutes; later builds reuse them.

Check that it works: open http://localhost:8080/api/health

```json
{"redis":"ok","replicas":1,"server":"ok"}
```

- `redis: "unreachable"` with status 503 → the backend cannot reach Redis.
- `replicas: 0` → the replica is not connected (yet).

Stop with `docker compose down`. The Redis data stays in Docker volumes; `docker compose down -v` deletes it.

## Tests

```
docker compose --profile test run --rm --build backend-tests
```

This builds the tests (GoogleTest) and runs them against the Redis primary from `compose.yaml`.
They use Redis **database 15** and empty it before every test; the site's data lives in database 0
and is never touched (the tests refuse to run on database 0).

| File | What it checks |
| --- | --- |
| `tests/text_test.cpp` | trimming, character counting (UTF-8), email normalising, ids, length limits |
| `tests/http_test.cpp` | reading cookies, the client IP with and without a trusted proxy, true/false and optional fields |
| `tests/datetime_test.cpp` | dates (leap years, 30 February), times, time zones, sorting numbers |
| `tests/crypto_test.cpp` | Argon2id hashes, password checks, random tokens, fingerprints |
| `tests/user_store_test.cpp` | registration rules, unique emails, login |
| `tests/session_store_test.cpp` | sessions: lookup, 30-day expiry, only fingerprints in Redis, logout |
| `tests/preset_store_test.cpp` | presets and quotes, deleting everything of a preset, other users get 404, active preset |
| `tests/rate_limiter_test.cpp` | limits, Retry-After, separate subjects, expiry, `check` and `reset` |
| `tests/event_store_test.cpp` | events: order by start, which events touch a range, rules, update, delete, other users get 404, the 2000 limit |
| `tests/reminder_store_test.cpp` | several reminders per user sorted by time, rules, partial changes, delete, other users get 404, the 20 limit |

If Redis cannot be reached, the Redis tests are skipped (not failed).

## Configuration

| Variable | Default | Meaning |
| --- | --- | --- |
| `PORT` | `8080` | Port the server listens on |
| `REDIS_HOST` | `127.0.0.1` | Redis primary host (`redis-primary` inside compose) |
| `REDIS_PORT` | `6379` | Redis primary port |
| `REDIS_PASSWORD` | none | Redis password, if the server has one |
| `COOKIE_SECURE` | `false` | `true` in production: the login cookie is sent only over HTTPS |
| `COOKIE_DOMAIN` | none | e.g. `.ubermenschs.com` in production, so the cookie works for the site and the API |
| `TRUST_PROXY` | `false` | `true` behind Render's proxy: the visitor's IP is read from `X-Forwarded-For`. Keep it `false` without a proxy, or anyone could fake their IP |

## Endpoints

| Method | Path | Body | Answer |
| --- | --- | --- | --- |
| GET | `/api/health` | | 200 if the server and Redis answer, 503 if Redis does not |
| POST | `/api/auth/register` | `{ email, username, password }` | 201 + the user, logged in (sets the cookie) |
| POST | `/api/auth/login` | `{ email, password }` | 200 + the user, logged in; 401 if wrong |
| POST | `/api/auth/logout` | | 204, logged out (clears the cookie) |
| GET | `/api/auth/me` | | 200 + the logged-in user, or 401 |
| GET | `/api/presets` | | the user's presets, with their quotes |
| POST | `/api/presets` | `{ name }` | 201 + the new preset |
| GET | `/api/presets/:id` | | one preset |
| PATCH | `/api/presets/:id` | `{ name }` | the renamed preset |
| DELETE | `/api/presets/:id` | | 204; its quotes are deleted too |
| POST | `/api/presets/:id/quotes` | `{ text, author }` | 201 + the new quote |
| DELETE | `/api/presets/:id/quotes/:quoteId` | | 204 |
| GET | `/api/settings` | | `{ activePreset }`: `"standard"` or a preset id |
| PUT | `/api/settings/active-preset` | `{ presetId }` | 204 |
| GET | `/api/events?from=YYYY-MM-DD&to=YYYY-MM-DD` | | the events touching those days (at most 400 days), by start |
| POST | `/api/events` | `{ title, start, end, allDay, description }` | 201 + the new event |
| PATCH | `/api/events/:id` | `{ title, start, end, allDay, description }` | the event (every field replaced) |
| DELETE | `/api/events/:id` | | 204 |
| GET | `/api/reminders` | | the user's reminders, by time of day |
| POST | `/api/reminders` | `{ time, message, timezone }` | 201 + the new reminder (on) |
| PATCH | `/api/reminders/:id` | any of `{ time, message, enabled, timezone }` | the reminder |
| DELETE | `/api/reminders/:id` | | 204 |

Errors come back as `{"error": "..."}` with 400 (bad data), 401 (not logged in), 404 (not found),
409 (email already used), 429 (too many attempts, with a `Retry-After` header in seconds) or 503 (Redis down).

The preset, event and reminder routes need a logged-in user; `GET /api/settings` answers `"standard"` for visitors.

Calendar and reminders:
- `start` and `end` are local times like `2026-11-06T14:30`; an all-day event goes from `T00:00` to `T23:59`.
- `time` is `HH:MM`; `timezone` is a name from the time zone database (`Europe/Bucharest`), checked with
  `std::chrono::locate_zone` (the Docker images install `tzdata` for it).
- A user can have up to 2000 events and 20 reminders.
- Redis keys: `event:{id}` (hash) and `user:{id}:events` (sorted set, the score is the start as a number);
  `reminder:{id}` (hash) and `user:{id}:reminders` (set).

How logins work:
- Passwords (at least 10 characters) are stored only as Argon2id hashes.
- A login creates a random 64-character token. The browser keeps it in the `session` cookie
  (HttpOnly, SameSite=Lax, 30 days); Redis keeps only a BLAKE2b fingerprint of it, under `session:{fingerprint}`.
- Logging out deletes the session in Redis and clears the cookie.

Rate limits (`src/rate_limiter.hpp`, fixed windows counted in Redis under `ratelimit:{name}:{subject}`):

| Limit | Counts | Max | Window |
| --- | --- | --- | --- |
| `login-ip` | every login attempt from one IP | 20 | 15 min |
| `login-fail` | wrong passwords for one email, from any IP; cleared by a successful login | 5 | 15 min |
| `register-ip` | new accounts from one IP | 5 | 1 hour |

A counter is increased and given its expiry in one Lua script, so a counter can never be left without one.

## Code

```
src/
  main.cpp             server start and /api/health (everything else is the "core" library)
  auth_routes.*        register, login, logout, me; the session cookie
  user_store.*         accounts and sessions in Redis
  crypto.*             libsodium: password hashes, random tokens, fingerprints
  route_helpers.hpp    who is logged in, and turning errors into HTTP answers
  rate_limiter.*       Redis counters that limit login and register attempts
  event_store.*        calendar events in Redis
  event_routes.*       the HTTP routes for events
  reminder_store.*     daily reminders in Redis
  reminder_routes.*    the HTTP routes for reminders
  datetime.*           checks for dates, times and time zones
  config.hpp/.cpp      settings from environment variables
  redis_client.*       the shared Redis client and the replica check
  preset_routes.*      the HTTP routes for presets and settings
  preset_store.*       reads and writes presets in Redis (keys, transactions, WAIT)
  http.*               JSON responses and reading request bodies
  text.*               trimming and length checks (counts characters, not bytes)
  errors.hpp           ValidationError 400, UnauthorizedError 401, NotFoundError 404, ConflictError 409,
                       TooManyRequestsError 429
tests/
  redis_fixture.hpp    connects the Redis tests to database 15 and empties it before each test
  *_test.cpp           the tests (see the table above)
```

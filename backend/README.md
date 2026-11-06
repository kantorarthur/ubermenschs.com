# ubermenschs.com — backend

C++20 server built with [Crow](https://crowcpp.org) (HTTP) and [redis-plus-plus](https://github.com/sewenew/redis-plus-plus) (Redis).
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

## Configuration

| Variable | Default | Meaning |
| --- | --- | --- |
| `PORT` | `8080` | Port the server listens on |
| `REDIS_HOST` | `127.0.0.1` | Redis primary host (`redis-primary` inside compose) |
| `REDIS_PORT` | `6379` | Redis primary port |
| `REDIS_PASSWORD` | none | Redis password, if the server has one |

## Endpoints

| Method | Path | Body | Answer |
| --- | --- | --- | --- |
| GET | `/api/health` | | 200 if the server and Redis answer, 503 if Redis does not |
| GET | `/api/presets` | | the user's presets, with their quotes |
| POST | `/api/presets` | `{ name }` | 201 + the new preset |
| GET | `/api/presets/:id` | | one preset |
| PATCH | `/api/presets/:id` | `{ name }` | the renamed preset |
| DELETE | `/api/presets/:id` | | 204; its quotes are deleted too |
| POST | `/api/presets/:id/quotes` | `{ text, author }` | 201 + the new quote |
| DELETE | `/api/presets/:id/quotes/:quoteId` | | 204 |
| GET | `/api/settings` | | `{ activePreset }`: `"standard"` or a preset id |
| PUT | `/api/settings/active-preset` | `{ presetId }` | 204 |

Errors come back as `{"error": "..."}` with 400 (bad data), 404 (not found) or 503 (Redis down).
Until login exists, every request belongs to the test user `1`.

Try it from PowerShell (with `docker compose up` running):

```
curl.exe -X POST -H "Content-Type: application/json" -d '{\"name\":\"Stoics\"}' http://localhost:8080/api/presets
curl.exe http://localhost:8080/api/presets
```

## Code

```
src/
  main.cpp             server start and /api/health
  config.hpp/.cpp      settings from environment variables
  redis_client.*       the shared Redis client and the replica check
  preset_routes.*      the HTTP routes for presets and settings
  preset_store.*       reads and writes presets in Redis (keys, transactions, WAIT)
  http.*               JSON responses and reading request bodies
  text.*               trimming and length checks (counts characters, not bytes)
  errors.hpp           ValidationError (400) and NotFoundError (404)
```

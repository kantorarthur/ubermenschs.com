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

| Method | Path | Answer |
| --- | --- | --- |
| GET | `/api/health` | 200 if the server and Redis answer, 503 if Redis does not |

## Code

```
src/
  main.cpp           routes and server start
  config.hpp/.cpp    settings from environment variables
  redis_client.*     the shared Redis client and the replica check
```

# ubermenschs.com

*One step closer to the Übermensch than you were yesterday.*

A web app for a daily practice of becoming better: a philosophical quote on every page, stories and Bible
passages worth rereading, a calendar and daily reminders that arrive as push notifications on the phone and
the computer. Live at **https://ubermenschs.com**.

## Tech stack

| Part | Technologies |
| --- | --- |
| **Backend** | **C++20** (GCC 13), [Crow](https://crowcpp.org) (HTTP server), CMake + Ninja, [vcpkg](https://vcpkg.io) (tag `2026.10.16`) |
| **Database** | **Redis 7** as the only database: AOF written every second + RDB snapshots; locally a primary and a replica |
| **Backend libraries** | [redis-plus-plus](https://github.com/sewenew/redis-plus-plus) 1.3 + hiredis (Redis client), [libsodium](https://libsodium.org) (Argon2id password hashes, random tokens, BLAKE2b), OpenSSL 3 (Web Push encryption and VAPID signatures), libcurl 8 (sending push notifications), C++20 `<chrono>` time zones (with `tzdata`) |
| **Frontend** | **React 19** + **TypeScript 7**, **Vite 8**, React Router 8, CSS Modules |
| **PWA** | Web App Manifest, a service worker (`public/sw.js`), Web Push (RFC 8030, encryption RFC 8291, VAPID RFC 8292), installable as an app |
| **Testing** | GoogleTest (backend, unit + Redis integration tests), Vitest + Testing Library + jsdom (frontend), oxlint |
| **Infrastructure** | **Docker** (multi-stage build on Ubuntu 24.04), **Docker Compose** (local), **Render** (backend + Redis on a persistent disk, Blueprint in `render.yaml`), **Cloudflare** (Pages for the site, DNS) |

## Features

- **Quotes in the header**: a random quote and background painting on every page, from the active quote set;
  users make their own sets.
- **Accounts**: register, log in, account details, change password (other devices are logged out).
  Passwords are stored as Argon2id hashes; sessions are random tokens in an HttpOnly cookie, and Redis keeps
  only their fingerprints.
- **Calendar**: events in a month view, each with optional notifications (at the start, 10 or 30 minutes,
  1 hour or 1 day before).
- **Daily reminders**: several per user, each at a fixed time in the user's time zone (summer and winter time
  included), sent as push notifications to every device where they are turned on.
- **Overview**: the main page with today's events and reminders and the way to every section.
- **Install as an app** ("Download application"): the browser's install window, or the steps for Safari and Firefox.
- **Security**: rate limits in Redis for login, registration, password changes and test notifications; CORS
  only for the site's own origins; push notifications only to the known push services.

## Preview

**1. Overview Page**

<img width="1877" height="987" alt="Image" src="https://github.com/user-attachments/assets/5aec6dcf-07f7-42c1-ba7e-e977dde98363" />

**2. Register/Login pages**

<img width="1871" height="988" alt="Image" src="https://github.com/user-attachments/assets/5fedb84d-48c8-41a9-a5a7-993a3b0a2285" />

<img width="1872" height="986" alt="Image" src="https://github.com/user-attachments/assets/1234f409-cdb1-4b89-9d25-3e704796435c" />

**3. Set Quotes page**

<img width="1873" height="988" alt="Image" src="https://github.com/user-attachments/assets/1529e60e-c1fb-48c9-a150-0aedb7a7ae05" />

**4. Phone notifications**

<img width="944" height="2046" alt="Image" src="https://github.com/user-attachments/assets/2de19a6c-d44a-4605-89a0-080da8aa4b7e" />

**5. Computer notifications**

<img width="331" height="227" alt="Image" src="https://github.com/user-attachments/assets/655ee967-e309-466b-b46c-414223609aef" />

**6. Calendar**

<img width="1873" height="988" alt="Image" src="https://github.com/user-attachments/assets/ec077a88-755c-4edf-a9c7-f4bec9da001f" />

**7. Love each other**

<img width="1874" height="987" alt="Image" src="https://github.com/user-attachments/assets/c681ba02-1e71-41d7-b6df-7e855b5627e6" />

**8. Reminder**

<img width="1874" height="980" alt="Image" src="https://github.com/user-attachments/assets/47237585-3c42-4942-97a8-354fc1971777" />

**9. Philosophical Study Plan**

<img width="1875" height="986" alt="Image" src="https://github.com/user-attachments/assets/65ac8ef2-761c-4df2-9821-6837dda5bf2a" />


## Architecture

```
 browser ──► https://ubermenschs.com          Cloudflare Pages   (frontend/: React, static files)
    │
    └──────► https://api.ubermenschs.com      Render web service (backend/: C++ server in Docker)
                                                 ├─ server   Crow, JSON API under /api, reminder scheduler thread
                                                 └─ Redis    127.0.0.1, data on a persistent disk (/data)
                                                     │
                                  push services ◄────┘  (Google FCM, Mozilla, Apple, Microsoft)
```

Locally, `docker compose` runs the backend with a separate Redis primary and replica, and Vite forwards `/api`
from http://localhost:5173 to the backend on port 8080.

## Repository layout

```
backend/            C++ server: src/, tests/, CMakeLists.txt, vcpkg.json, Dockerfile, docker-entrypoint.sh
frontend/           React + TypeScript app: src/, public/ (service worker, manifest, icons)
compose.yaml        local backend + Redis primary + Redis replica (+ the test runner)
render.yaml         Render Blueprint for production
DEPLOY.md           step-by-step deployment (Render + Cloudflare)
```

More detail: [backend/README.md](backend/README.md) (API endpoints, Redis keys, configuration),
[frontend/README.md](frontend/README.md), [DEPLOY.md](DEPLOY.md).

## Run it locally

Requires Docker Desktop and Node.js 20.19+ or 22.12+.

```
docker compose up --build
```

Backend on http://localhost:8080 (check http://localhost:8080/api/health). Then, in a second terminal:

```
cd frontend
npm install
npm run dev
```

The site is on http://localhost:5173.

## Tests

```
docker compose --profile test run --rm --build backend-tests
```

```
cd frontend
npm test
npm run lint
npm run build
```

The backend's Redis tests use database 15 and never touch the site's data (database 0).

## Test the production container before pushing

The image Render runs, with Redis inside it, can be started locally the same way:

```
docker compose down
docker build -t ubermenschs-api ./backend
docker run --rm -p 8080:8080 -e EMBEDDED_REDIS=true -e PORT=8080 -v ubermenschs-test-data:/data ubermenschs-api
```

`/api/health` should answer `{"redis":"ok","replicas":0,"server":"ok"}`. Stop it with Ctrl+C and start it again:
the data must still be there. If a deploy still goes wrong, Render (**Deploys → Rollback**) and Cloudflare Pages
(**Deployments → Rollback to this deployment**) go back to the previous version in a minute.

## Co-author

Claude Code

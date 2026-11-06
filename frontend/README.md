# ubermenschs.com — frontend

React + TypeScript, built with Vite.

## Run it locally

Requires Node.js 20.19+ or 22.12+ (`node -v`).

```
npm install
npm run dev
```

Then open http://localhost:5173.

## Commands

| Command | What it does |
| --- | --- |
| `npm run dev` | Development server with instant reload |
| `npm run build` | Type-check and build the static site into `dist/` |
| `npm run preview` | Serve the built `dist/` locally |
| `npm run lint` | Check the code with oxlint |
| `npm test` | Run the tests once (Vitest) |
| `npm run test:watch` | Run the tests again on every change |

## Where things are

```
src/
  main.tsx        entry point (router setup)
  App.tsx         the list of routes
  layout/         AppShell, Logo, QuoteBar, AuthButtons, Sidebar
  pages/          one page per route
  components/     shared pieces (Page, NotificationSettings, ...)
  hooks/          useRandomOnChange, useMediaQuery
  data/           fixed content (standard quotes, background list)
  types/          data types shared with the C++ backend
  assets/backgrounds/landscape/   backgrounds for wide screens
  assets/backgrounds/portrait/    backgrounds for phones
  utils/          dates, notifications
public/
  sw.js, manifest.webmanifest, icons/   the PWA files (see Notifications below)
```

Tests sit next to the code they test, as `*.test.ts` / `*.test.tsx` (dates, the random pick, the API client,
the auth, presets, events, reminders and push API, the notifications helpers, QuoteBar, NotificationSettings).
They run in a simulated browser (jsdom) and never call the real backend: `fetch` is replaced by prepared answers.

To add a background, put a `.webp` file into `landscape/` or `portrait/`; no code change is needed.

## Notifications (PWA)

- `public/manifest.webmanifest` and `public/icons/` let phones install the site like an app.
- `public/sw.js` is the service worker: it shows the reminder notifications, also when no tab is open,
  and opens the Reminder page when one is clicked.
- `src/utils/install.ts` and the "Download application" button (`src/layout/InstallButton.tsx`) install the site
  as an app: the browser's own install window in Chrome and Edge, written steps in Safari and Firefox.
- `src/utils/notifications.ts` asks for permission and subscribes the browser; the panel on the Reminder page
  (`src/components/NotificationSettings.tsx`) has the buttons.
- Browsers allow this only on `https://` sites and on `localhost`. Locally, use Chrome, Edge or Firefox in a
  normal (not private) window. On iPhone and iPad it works only after adding the site to the home screen.

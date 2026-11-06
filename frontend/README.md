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

## Where things are

```
src/
  main.tsx        entry point (router setup)
  App.tsx         the list of routes
  layout/         AppShell, Logo, QuoteBar, AuthButtons, Sidebar
  pages/          one page per route
  components/     shared pieces (Page)
  hooks/          useRandomOnChange, useMediaQuery
  data/           fixed content (standard quotes, background list)
  types/          data types shared with the C++ backend
  assets/backgrounds/landscape/   backgrounds for wide screens
  assets/backgrounds/portrait/    backgrounds for phones
```

To add a background, put a `.webp` file into `landscape/` or `portrait/`; no code change is needed.

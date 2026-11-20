# Deploying ubermenschs.com

```
 browser ──► https://ubermenschs.com          Cloudflare Pages   (frontend/, free)
    │
    └──────► https://api.ubermenschs.com      Render web service (backend/, Docker, $7/month + disk)
                                                 ├─ server  (C++)
                                                 └─ Redis   (127.0.0.1, data on a 1 GB disk at /data)
```

- The domain and its DNS are at Cloudflare.
- `render.yaml` describes the backend; Render creates it, with its disk, from it.
- Redis runs in the backend's container (`backend/docker-entrypoint.sh`, `EMBEDDED_REDIS=true`): AOF written
  every second plus snapshots, on the disk. Render snapshots the disk every 24 hours and keeps 7 days of them.
- With a disk, a deploy first stops the old version and then starts the new one: the API is down for a few
  seconds. A reminder due in that moment is sent when the server is back (up to 10 minutes late).
- The login cookie is set for `.ubermenschs.com`, so it works for the site and the API.
- The API only answers the browser for the origins in `ALLOWED_ORIGINS` (CORS).

## 1. Make the VAPID keys (once)

The key pair that signs push notifications. Make it once and keep it: a new pair turns off notifications on
every device until they are turned on again.

```
docker compose run --rm backend server --generate-vapid-keys
```

It prints `VAPID_PUBLIC_KEY=...` and `VAPID_PRIVATE_KEY=...`. Keep them for step 2.
Do not put them in any file of the repository; the private key is a secret, like a password.

## 2. Backend on Render

1. Push the code to GitHub (`main`).
2. Render Dashboard → **New** → **Blueprint** → choose the `ubermenschs.com` repository.
3. Render shows the service from `render.yaml`: `ubermenschs-api`, with the disk `redis-data`.
   It asks for `VAPID_PUBLIC_KEY` and `VAPID_PRIVATE_KEY`: paste the values from step 1.
4. **Apply**. The first build compiles the C++ libraries (vcpkg) and takes a while
   (Render cancels builds after 120 minutes; the Hobby plan includes 500 build minutes per month).
5. When it is live, open `https://ubermenschs-api.onrender.com/api/health` (the exact address is on the
   service's page). Expected: `{"redis":"ok","replicas":0,"server":"ok"}`.

## 3. api.ubermenschs.com

1. Render → `ubermenschs-api` → **Settings** → **Custom Domains** → add `api.ubermenschs.com`.
2. Cloudflare → `ubermenschs.com` → **DNS** → **Records** → add:
   - Type `CNAME`, Name `api`, Target `ubermenschs-api.onrender.com` (the address from step 2.5),
   - Proxy status **DNS only** (grey cloud).

   Keep it **DNS only**. With the orange cloud, every request would reach Render from a Cloudflare address,
   and the backend would count login attempts per Cloudflare server instead of per visitor.
3. Back in Render, wait until the domain shows as verified and the certificate as issued.
   `https://api.ubermenschs.com/api/health` should then answer.

## 4. The site on Cloudflare Pages

1. Cloudflare → **Workers & Pages** → **Create** → **Pages** → **Connect to Git** → the repository.
2. Build settings:
   - Production branch: `main`
   - Framework preset: `None` (or Vite)
   - Root directory: `frontend`
   - Build command: `npm run build`
   - Build output directory: `dist`
3. Environment variables (Production):
   - `VITE_API_URL` = `https://api.ubermenschs.com`
   - `NODE_VERSION` = `22`
4. **Save and Deploy**.
5. The project → **Custom domains** → add `ubermenschs.com`, then `www.ubermenschs.com`.
   The domain is in the same Cloudflare account, so Cloudflare adds the DNS records itself.

Pages serves `index.html` for every path that is not a file, so links like `/reminders` work on refresh.

## 5. Check

- `https://ubermenschs.com` opens, the header quote and background change between pages.
- Register, log in, reload: still logged in.
- Presets, calendar events and reminders stay after reloading.
- Reminder → **Turn on notifications** → **Send a test notification**, on the computer and on the phone.
  On iPhone/iPad: Share → **Add to Home Screen**, open the site from the home screen, then turn them on.
- A faked `X-Forwarded-For` header cannot get around the login limit (20 attempts per 15 minutes per visitor).
  In PowerShell, 21 attempts with a different fake address and a different email each time:

  ```
  1..21 | ForEach-Object {
      $body = @{ email = "nobody$_@example.com"; password = "wrong password" } | ConvertTo-Json
      try
      {
          Invoke-WebRequest -UseBasicParsing -Method Post -Uri https://api.ubermenschs.com/api/auth/login `
              -ContentType "application/json" -Headers @{ "X-Forwarded-For" = "10.0.0.$_" } -Body $body | Out-Null
      }
      catch
      {
          Write-Host -NoNewline "$([int]$_.Exception.Response.StatusCode) "
      }
  }
  ```

  Expected: twenty `401` and then `429`. Twenty-one `401` would mean the fake addresses were believed.
  This uses up your own login attempts for 15 minutes.

## Updating

- A push to `main` that changes `backend/` redeploys the backend; one that changes `frontend/` redeploys the site.
- Redis keeps its data across deploys and restarts: it is on the disk, and a normal stop writes everything.
  If Render's machine fails, at most the last second of writes can be lost.
- To go back to a disk snapshot: Render → `ubermenschs-api` → **Disks** → snapshots (kept 7 days).

## Costs (Render prices checked in November 2026)

| What | Plan | Price |
| --- | --- | --- |
| Backend + Redis | `0.5c-512mb` (does not sleep) | $7/month |
| Disk for Redis | 1 GB | see the price in the Render Dashboard when adding it |
| Site | Cloudflare Pages | free |

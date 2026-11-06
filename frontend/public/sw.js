// The service worker: a small script the browser keeps running for the site in the background,
// so a notification can appear even when no tab of ubermenschs.com is open.
//
// The backend sends {"title", "body", "tag", "url"} (see backend/src/reminder_scheduler.hpp).
// It has to be a plain file in public/, served at /sw.js, so it can act for the whole site.

self.addEventListener('install', () => {
  // Use a new version of this file right away instead of waiting for every tab to close.
  self.skipWaiting()
})

self.addEventListener('activate', (event) => {
  event.waitUntil(self.clients.claim())
})

self.addEventListener('push', (event) => {
  let data = {}
  if (event.data) {
    try {
      data = event.data.json()
    } catch {
      data = { body: event.data.text() }
    }
  }

  event.waitUntil(
    self.registration.showNotification(data.title || 'ubermenschs.com', {
      body: data.body || '',
      // A newer notification with the same tag replaces the older one instead of piling up.
      tag: data.tag,
      icon: '/icons/icon-192.png',
      badge: '/icons/icon-192.png',
      data: { url: data.url || '/' },
    }),
  )
})

// Clicking the notification opens the page it is about, in an open tab of the site if there is one.
self.addEventListener('notificationclick', (event) => {
  event.notification.close()
  const url = new URL(event.notification.data?.url || '/', self.location.origin).href

  event.waitUntil(
    (async () => {
      const windows = await self.clients.matchAll({ type: 'window', includeUncontrolled: true })
      for (const client of windows) {
        if (client.url.startsWith(self.location.origin)) {
          await client.focus()
          if ('navigate' in client) {
            await client.navigate(url)
          }
          return
        }
      }
      await self.clients.openWindow(url)
    })(),
  )
})

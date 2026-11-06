import { apiFetch } from './client.ts'

// Push notifications: which browsers of the user get the reminders.
// Every function is one request to the backend (backend/src/push_routes.hpp).

// GET /api/push/public-key — the server's VAPID key; the browser needs it to subscribe.
export async function getPushPublicKey(): Promise<string> {
  const answer = await apiFetch<{ publicKey: string }>('/api/push/public-key')
  return answer.publicKey
}

// POST /api/push/subscriptions — body: PushSubscription.toJSON() from the browser.
export function saveSubscription(subscription: PushSubscriptionJSON): Promise<{ id: string }> {
  return apiFetch<{ id: string }>('/api/push/subscriptions', { method: 'POST', body: subscription })
}

// POST /api/push/unsubscribe
export function removeSubscription(endpoint: string): Promise<void> {
  return apiFetch<void>('/api/push/unsubscribe', { method: 'POST', body: { endpoint } })
}

// POST /api/push/test — a test notification to every browser of the user.
export function sendTestNotification(): Promise<{ sent: number; devices: number }> {
  return apiFetch<{ sent: number; devices: number }>('/api/push/test', { method: 'POST' })
}

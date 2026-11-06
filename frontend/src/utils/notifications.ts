import { getPushPublicKey, removeSubscription, saveSubscription } from '../api/push.ts'

// The browser side of push notifications: the service worker (public/sw.js), the permission
// and the subscription the browser gets from its push service.

// unsupported: the browser cannot get push notifications (e.g. Safari on iPhone outside the home screen)
// denied:      the user blocked notifications for this site
// off:         possible, but not turned on in this browser
// on:          this browser gets the reminders
export type NotificationState = 'unsupported' | 'denied' | 'off' | 'on'

export function pushSupported(): boolean {
  return typeof window !== 'undefined' && 'serviceWorker' in navigator && 'PushManager' in window && 'Notification' in window
}

// Registers public/sw.js (main.tsx calls this once) and waits until it is active.
export async function serviceWorker(): Promise<ServiceWorkerRegistration> {
  await navigator.serviceWorker.register('/sw.js')
  return navigator.serviceWorker.ready
}

// The VAPID key is sent as base64url text, but subscribe() wants the bytes.
export function base64UrlToBytes(text: string): Uint8Array<ArrayBuffer> {
  const base64 = text.replace(/-/g, '+').replace(/_/g, '/') + '='.repeat((4 - (text.length % 4)) % 4)
  const binary = atob(base64)
  const bytes = new Uint8Array(binary.length)
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i)
  }
  return bytes
}

function sameBytes(a: ArrayBuffer | null, b: Uint8Array): boolean {
  if (!a || a.byteLength !== b.length) {
    return false
  }
  const view = new Uint8Array(a)
  return view.every((value, i) => value === b[i])
}

export async function notificationState(): Promise<NotificationState> {
  if (!pushSupported()) {
    return 'unsupported'
  }
  if (Notification.permission === 'denied') {
    return 'denied'
  }
  if (Notification.permission !== 'granted') {
    return 'off'
  }
  const registration = await serviceWorker()
  return (await registration.pushManager.getSubscription()) ? 'on' : 'off'
}

// Asks for permission (the browser shows its own question), subscribes this browser and tells the backend.
export async function turnOnNotifications(): Promise<void> {
  const permission = await Notification.requestPermission()
  if (permission === 'denied') {
    throw new Error('Notifications are blocked for this site. Allow them in the browser settings, then try again.')
  }
  if (permission !== 'granted') {
    throw new Error('Notifications were not allowed.')
  }

  const registration = await serviceWorker()
  const key = base64UrlToBytes(await getPushPublicKey())

  let subscription = await registration.pushManager.getSubscription()
  // A subscription made for another server key would be refused by the push service: replace it.
  if (subscription && !sameBytes(subscription.options.applicationServerKey, key)) {
    await subscription.unsubscribe()
    subscription = null
  }
  subscription ??= await subscribe(registration, key)
  await saveSubscription(subscription.toJSON())
}

// The browser asks its push service (Google, Mozilla, Apple, ...) for an address for this site.
async function subscribe(registration: ServiceWorkerRegistration, key: Uint8Array<ArrayBuffer>): Promise<PushSubscription> {
  let timer: ReturnType<typeof setTimeout> | undefined
  const timeout = new Promise<never>((_, reject) => {
    timer = setTimeout(() => reject(new Error("The browser's push service did not answer. Try again later.")), 30_000)
  })
  try {
    return await Promise.race([
      registration.pushManager.subscribe({ userVisibleOnly: true, applicationServerKey: key }),
      timeout,
    ])
  } catch (e) {
    if (e instanceof DOMException) {
      // e.g. Chrome in an incognito window: "Registration failed - permission denied"
      throw new Error(`The browser could not turn on notifications (${e.message}). Private windows cannot get them.`)
    }
    throw e
  } finally {
    clearTimeout(timer)
  }
}

export async function turnOffNotifications(): Promise<void> {
  const registration = await serviceWorker()
  const subscription = await registration.pushManager.getSubscription()
  if (subscription) {
    await removeSubscription(subscription.endpoint)
    await subscription.unsubscribe()
  }
}

// Tells the backend again which account this browser belongs to, e.g. after logging in.
// Does nothing if notifications are not on in this browser.
export async function syncSubscription(): Promise<void> {
  if ((await notificationState()) !== 'on') {
    return
  }
  const registration = await serviceWorker()
  const subscription = await registration.pushManager.getSubscription()
  if (subscription) {
    await saveSubscription(subscription.toJSON())
  }
}

// Before logging out: the backend stops sending this account's reminders to this browser.
// The browser keeps its subscription, so logging in again turns them back on (syncSubscription).
export async function forgetSubscriptionOnServer(): Promise<void> {
  if (!pushSupported() || Notification.permission !== 'granted') {
    return
  }
  const registration = await navigator.serviceWorker.getRegistration()
  const subscription = await registration?.pushManager.getSubscription()
  if (subscription) {
    await removeSubscription(subscription.endpoint)
  }
}

import { useSyncExternalStore } from 'react'

// The answer to the cookie notice (layout/CookieBanner.tsx).
//
// The site's only cookie is the login cookie, so the answer matters for logging in: without "Accept",
// visitors can read every page, but the login and registration forms stay off.
// "Accept" is remembered in localStorage together with CONSENT_VERSION; when the privacy page changes,
// a new version makes the notice ask again. "Not now" is remembered only until the page is reloaded.

export const CONSENT_VERSION = '2026-11'
const STORAGE_KEY = 'cookie-consent'

export interface CookieConsent {
  accepted: boolean
  // "Not now" was clicked: the notice stays hidden, except on the login and registration pages.
  dismissed: boolean
}

// localStorage can be missing or throw (private windows, blocked site data): then the answer is
// kept only in memory, and the notice comes back after a reload.
function readStored(): boolean {
  try {
    return window.localStorage.getItem(STORAGE_KEY) === CONSENT_VERSION
  } catch {
    return false
  }
}

let state: CookieConsent = { accepted: typeof window !== 'undefined' && readStored(), dismissed: false }
const listeners = new Set<() => void>()

function update(next: CookieConsent) {
  state = next
  for (const listener of listeners) {
    listener()
  }
}

export function acceptCookie() {
  try {
    window.localStorage.setItem(STORAGE_KEY, CONSENT_VERSION)
  } catch {
    // Kept in memory only; see readStored.
  }
  update({ accepted: true, dismissed: false })
}

export function dismissCookieNotice() {
  update({ ...state, dismissed: true })
}

// Reads localStorage again and forgets "Not now", as after a page reload (used by the tests).
export function reloadCookieConsent() {
  update({ accepted: readStored(), dismissed: false })
}

function subscribe(listener: () => void) {
  listeners.add(listener)
  return () => {
    listeners.delete(listener)
  }
}

export function useCookieConsent(): CookieConsent {
  return useSyncExternalStore(subscribe, () => state)
}

import { useSyncExternalStore } from 'react'

// Installing the site as an app (a PWA: its own window and icon, no App Store).
//
// Chrome, Edge, Samsung Internet and Opera offer it with the "beforeinstallprompt" event, which can
// arrive before React has started, so this file listens from the moment it is imported (main.tsx).
// Safari (iPhone, iPad, Mac) and Firefox have no such event: there the user installs from the
// browser's own menu, and the button explains how.

// Not in TypeScript's DOM types yet.
interface BeforeInstallPromptEvent extends Event {
  prompt(): Promise<void>
  userChoice: Promise<{ outcome: 'accepted' | 'dismissed' }>
}

export type InstallHelp = 'ios' | 'mac-safari' | 'firefox' | 'other'

let promptEvent: BeforeInstallPromptEvent | null = null
let installedNow = false
const listeners = new Set<() => void>()

function changed() {
  for (const listener of listeners) {
    listener()
  }
}

if (typeof window !== 'undefined') {
  window.addEventListener('beforeinstallprompt', (event) => {
    // Keep it for our button instead of letting the browser show its own small banner.
    event.preventDefault()
    promptEvent = event as BeforeInstallPromptEvent
    changed()
  })
  window.addEventListener('appinstalled', () => {
    promptEvent = null
    installedNow = true
    changed()
  })
}

// True when the site already runs as an installed app (opened from its icon).
export function isRunningInstalled(): boolean {
  if (typeof window === 'undefined') {
    return false
  }
  const iosStandalone = (navigator as Navigator & { standalone?: boolean }).standalone === true
  return iosStandalone || window.matchMedia?.('(display-mode: standalone)').matches === true
}

// Which instructions fit this browser when it cannot show the install window itself.
export function installHelp(userAgent: string, maxTouchPoints: number): InstallHelp {
  const iPad = /Macintosh/.test(userAgent) && maxTouchPoints > 1 // iPadOS says it is a Mac
  if (/iPhone|iPad|iPod/.test(userAgent) || iPad) {
    return 'ios'
  }
  if (/Firefox\//.test(userAgent)) {
    return 'firefox'
  }
  if (/Macintosh/.test(userAgent) && /Safari\//.test(userAgent) && !/Chrome\/|Chromium\/|Edg\//.test(userAgent)) {
    return 'mac-safari'
  }
  return 'other'
}

interface InstallState {
  installed: boolean
  canPrompt: boolean
}

let snapshot: InstallState = { installed: false, canPrompt: false }

function currentState(): InstallState {
  const next = { installed: installedNow || isRunningInstalled(), canPrompt: promptEvent !== null }
  // The same object while nothing changed, as useSyncExternalStore requires.
  if (next.installed !== snapshot.installed || next.canPrompt !== snapshot.canPrompt) {
    snapshot = next
  }
  return snapshot
}

function subscribe(listener: () => void) {
  listeners.add(listener)
  return () => listeners.delete(listener)
}

export function useInstallState(): InstallState {
  return useSyncExternalStore(subscribe, currentState, currentState)
}

// Shows the browser's install window. Returns false if this browser has none (see installHelp).
export async function promptInstall(): Promise<boolean> {
  if (!promptEvent) {
    return false
  }
  const event = promptEvent
  // The event can be used only once.
  promptEvent = null
  changed()
  await event.prompt()
  const choice = await event.userChoice
  if (choice.outcome === 'accepted') {
    installedNow = true
    changed()
  }
  return true
}

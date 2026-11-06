import type { Reminder } from '../types/models.ts'
import { apiFetch } from './client.ts'

// The user's daily reminders (a user can have several), stored by the C++ backend in Redis.
// Every function is one request to the API described in "Contractul API" in the plan.

// The backend checks the same limits; here they only set the inputs' maxLength.
export const REMINDER_LIMITS = {
  message: 200,
}

// The browser's time zone, e.g. "Europe/Bucharest". The backend needs it to send
// the notification at the right moment, also after the switch to summer or winter time.
export function browserTimeZone(): string {
  return Intl.DateTimeFormat().resolvedOptions().timeZone
}

// GET /api/reminders   (sorted by time of day)
export function getReminders(): Promise<Reminder[]> {
  return apiFetch<Reminder[]>('/api/reminders')
}

// POST /api/reminders   — a new reminder starts on, in the browser's time zone
export function createReminder(input: { time: string; message: string }): Promise<Reminder> {
  return apiFetch<Reminder>('/api/reminders', {
    method: 'POST',
    body: { ...input, timezone: browserTimeZone() },
  })
}

// PATCH /api/reminders/:id   — only the given fields change.
// A new time is meant in the browser's current time zone, so the zone is sent along with it.
export function updateReminder(
  reminderId: string,
  changes: { time?: string; message?: string; enabled?: boolean },
): Promise<Reminder> {
  const body = changes.time === undefined ? changes : { ...changes, timezone: browserTimeZone() }
  return apiFetch<Reminder>(`/api/reminders/${encodeURIComponent(reminderId)}`, { method: 'PATCH', body })
}

// DELETE /api/reminders/:id
export function deleteReminder(reminderId: string): Promise<void> {
  return apiFetch<void>(`/api/reminders/${encodeURIComponent(reminderId)}`, { method: 'DELETE' })
}

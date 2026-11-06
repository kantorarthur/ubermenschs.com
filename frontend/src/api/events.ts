import type { CalendarEvent } from '../types/models.ts'
import { apiFetch } from './client.ts'
import { browserTimeZone } from './reminders.ts'

// The user's calendar events, stored by the C++ backend in Redis.
// Every function is one request to the API described in "Contractul API" in the plan.

// What the form sends; the time zone is added here (the backend needs it for the notifications).
export type EventInput = Omit<CalendarEvent, 'id' | 'timezone'>

// The notification choices, in minutes before the start (the backend accepts exactly these).
export const NOTIFY_OPTIONS = [
  { minutes: 0, label: 'At the start' },
  { minutes: 10, label: '10 minutes before' },
  { minutes: 30, label: '30 minutes before' },
  { minutes: 60, label: '1 hour before' },
  { minutes: 1440, label: '1 day before' },
]

// All-day events have no time: their notifications count from 09:00 on the day.
export const ALL_DAY_NOTIFY_OPTIONS = [
  { minutes: 0, label: 'On the day, at 09:00' },
  { minutes: 1440, label: 'The day before, at 09:00' },
]

// The backend checks the same limits; here they only set the inputs' maxLength.
export const EVENT_LIMITS = {
  title: 100,
  description: 500,
}

// GET /api/events?from=…&to=…   (from and to are "YYYY-MM-DD", both included)
// Returns every event that touches at least one day in the range, sorted by start.
export function getEvents(from: string, to: string): Promise<CalendarEvent[]> {
  const query = new URLSearchParams({ from, to })
  return apiFetch<CalendarEvent[]>(`/api/events?${query.toString()}`)
}

// POST /api/events
export function createEvent(input: EventInput): Promise<CalendarEvent> {
  return apiFetch<CalendarEvent>('/api/events', { method: 'POST', body: { ...input, timezone: browserTimeZone() } })
}

// PATCH /api/events/:id   (every field is replaced)
export function updateEvent(eventId: string, input: EventInput): Promise<CalendarEvent> {
  return apiFetch<CalendarEvent>(`/api/events/${encodeURIComponent(eventId)}`, {
    method: 'PATCH',
    body: { ...input, timezone: browserTimeZone() },
  })
}

// DELETE /api/events/:id
export function deleteEvent(eventId: string): Promise<void> {
  return apiFetch<void>(`/api/events/${encodeURIComponent(eventId)}`, { method: 'DELETE' })
}

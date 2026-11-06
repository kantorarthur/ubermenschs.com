import type { CalendarEvent } from '../types/models.ts'
import { apiFetch } from './client.ts'

// The user's calendar events, stored by the C++ backend in Redis.
// Every function is one request to the API described in "Contractul API" in the plan.

export type EventInput = Omit<CalendarEvent, 'id'>

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
  return apiFetch<CalendarEvent>('/api/events', { method: 'POST', body: input })
}

// PATCH /api/events/:id   (every field is replaced)
export function updateEvent(eventId: string, input: EventInput): Promise<CalendarEvent> {
  return apiFetch<CalendarEvent>(`/api/events/${encodeURIComponent(eventId)}`, { method: 'PATCH', body: input })
}

// DELETE /api/events/:id
export function deleteEvent(eventId: string): Promise<void> {
  return apiFetch<void>(`/api/events/${encodeURIComponent(eventId)}`, { method: 'DELETE' })
}

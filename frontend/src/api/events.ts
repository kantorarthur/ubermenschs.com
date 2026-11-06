import type { CalendarEvent } from '../types/models.ts'
import { dateKeyOf } from '../utils/dates.ts'

// The user's calendar events.
//
// TEMPORARY: the data lives in memory, so it disappears on refresh.
// Every function already has the shape of the real API call (see "Contractul API" in the plan).

export type EventInput = Omit<CalendarEvent, 'id'>

export const EVENT_LIMITS = {
  title: 100,
  description: 500,
}

let events: CalendarEvent[] = []
let nextId = 1

function networkDelay(): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, 120))
}

function copy<T>(value: T): T {
  return structuredClone(value)
}

function findEvent(eventId: string): CalendarEvent {
  const event = events.find((e) => e.id === eventId)
  if (!event) {
    throw new Error('This event does not exist.')
  }
  return event
}

// The same checks the backend will make; returns the cleaned-up event.
function validate(input: EventInput): EventInput {
  const title = input.title.trim()
  const description = input.description.trim()

  if (title.length === 0) {
    throw new Error('The event needs a title.')
  }
  if (title.length > EVENT_LIMITS.title) {
    throw new Error(`The title can have at most ${EVENT_LIMITS.title} characters.`)
  }
  if (description.length > EVENT_LIMITS.description) {
    throw new Error(`The description can have at most ${EVENT_LIMITS.description} characters.`)
  }
  // "YYYY-MM-DDTHH:mm" strings sort the same way as the moments they describe.
  if (input.end < input.start) {
    throw new Error('The event cannot end before it starts.')
  }

  return { ...input, title, description }
}

// GET /api/events?from=…&to=…   (from and to are "YYYY-MM-DD", both included)
// Returns every event that touches at least one day in the range, sorted by start.
export async function getEvents(from: string, to: string): Promise<CalendarEvent[]> {
  await networkDelay()
  const inRange = events.filter((e) => dateKeyOf(e.start) <= to && dateKeyOf(e.end) >= from)
  return copy(inRange.sort((a, b) => a.start.localeCompare(b.start)))
}

// POST /api/events
export async function createEvent(input: EventInput): Promise<CalendarEvent> {
  await networkDelay()
  const event: CalendarEvent = { id: '', ...validate(input) }
  event.id = `event-${nextId}`
  nextId += 1
  events.push(event)
  return copy(event)
}

// PATCH /api/events/:id
export async function updateEvent(eventId: string, input: EventInput): Promise<CalendarEvent> {
  await networkDelay()
  const event = findEvent(eventId)
  Object.assign(event, validate(input))
  return copy(event)
}

// DELETE /api/events/:id
export async function deleteEvent(eventId: string): Promise<void> {
  await networkDelay()
  findEvent(eventId)
  events = events.filter((e) => e.id !== eventId)
}

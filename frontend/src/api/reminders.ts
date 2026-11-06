import type { Reminder } from '../types/models.ts'

// The user's daily reminders.
//
// TEMPORARY: the data lives in memory, so it disappears on refresh.
// Every function already has the shape of the real API call (see "Contractul API" in the plan).

export const REMINDER_LIMITS = {
  message: 200,
}

let reminders: Reminder[] = []
let nextId = 1

function networkDelay(): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, 120))
}

function copy<T>(value: T): T {
  return structuredClone(value)
}

// The browser's time zone, e.g. "Europe/Bucharest". The backend needs it to send
// the notification at the right moment, also after the switch to summer or winter time.
export function browserTimeZone(): string {
  return Intl.DateTimeFormat().resolvedOptions().timeZone
}

function findReminder(reminderId: string): Reminder {
  const reminder = reminders.find((r) => r.id === reminderId)
  if (!reminder) {
    throw new Error('This reminder does not exist.')
  }
  return reminder
}

function checkTime(time: string): string {
  // "HH:MM", 00:00 to 23:59
  if (!/^([01]\d|2[0-3]):[0-5]\d$/.test(time)) {
    throw new Error('Choose a time.')
  }
  return time
}

function checkMessage(message: string): string {
  const trimmed = message.trim()
  if (trimmed.length === 0) {
    throw new Error('The reminder needs a message.')
  }
  if (trimmed.length > REMINDER_LIMITS.message) {
    throw new Error(`The message can have at most ${REMINDER_LIMITS.message} characters.`)
  }
  return trimmed
}

// GET /api/reminders   (sorted by time of day)
export async function getReminders(): Promise<Reminder[]> {
  await networkDelay()
  return copy([...reminders].sort((a, b) => a.time.localeCompare(b.time)))
}

// POST /api/reminders
export async function createReminder(input: { time: string; message: string }): Promise<Reminder> {
  await networkDelay()
  const time = checkTime(input.time)
  const message = checkMessage(input.message)
  const reminder: Reminder = {
    id: `reminder-${nextId}`,
    time,
    message,
    enabled: true,
    timezone: browserTimeZone(),
  }
  nextId += 1
  reminders.push(reminder)
  return copy(reminder)
}

// PATCH /api/reminders/:id
export async function updateReminder(
  reminderId: string,
  changes: { time?: string; message?: string; enabled?: boolean },
): Promise<Reminder> {
  await networkDelay()
  const reminder = findReminder(reminderId)
  if (changes.time !== undefined) {
    reminder.time = checkTime(changes.time)
  }
  if (changes.message !== undefined) {
    reminder.message = checkMessage(changes.message)
  }
  if (changes.enabled !== undefined) {
    reminder.enabled = changes.enabled
  }
  return copy(reminder)
}

// DELETE /api/reminders/:id
export async function deleteReminder(reminderId: string): Promise<void> {
  await networkDelay()
  findReminder(reminderId)
  reminders = reminders.filter((r) => r.id !== reminderId)
}

// Data types shared with the C++ backend (see "Contractul API" in the plan).
// The JSON the backend sends must have exactly these shapes.

export interface User {
  id: string
  email: string
  username: string
  createdAt: number // when the account was made, in seconds since 1970 (UTC)
}

export interface Quote {
  id: string
  text: string
  author: string
}

export interface QuotePreset {
  id: string
  name: string
  quotes: Quote[]
}

export interface Story {
  slug: string
  title: string
  philosopher: string
  text: string
}

// A Bible passage on the "Love each other as I have loved you." page (fixed, in the frontend).
export interface ScripturePassage {
  id: string
  reference: string // e.g. "John 15:12"
  text: string // may contain line breaks between paragraphs
}

export interface CalendarEvent {
  id: string
  title: string
  start: string // local date-time "2026-11-06T14:30"; all-day events start at T00:00
  end: string // local date-time "2026-11-06T15:30"; all-day events end at T23:59
  allDay: boolean
  description: string
  notify: number[] // minutes before the start to send a notification: any of 0, 10, 30, 60, 1440
  timezone: string // the browser's time zone when the event was saved, e.g. "Europe/Bucharest"
}

export interface Reminder {
  id: string
  time: string // "07:30"
  message: string
  enabled: boolean
  timezone: string // e.g. "Europe/Bucharest"
}

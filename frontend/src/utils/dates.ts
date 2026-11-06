// Small date helpers for the calendar.
//
// Dates are handled as local "YYYY-MM-DD" keys and "YYYY-MM-DDTHH:mm" date-times, the same
// format as <input type="date"> and <input type="time"> produce. We never use toISOString()
// here, because it converts to UTC and can move a date to the previous or next day.

export const WEEKDAYS = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']

const MONTHS = [
  'January',
  'February',
  'March',
  'April',
  'May',
  'June',
  'July',
  'August',
  'September',
  'October',
  'November',
  'December',
]

function pad(n: number): string {
  return String(n).padStart(2, '0')
}

// Date → "2026-11-06"
export function toDateKey(date: Date): string {
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`
}

// "2026-11-06T14:30" → "2026-11-06"
export function dateKeyOf(dateTime: string): string {
  return dateTime.slice(0, 10)
}

// "2026-11-06T14:30" → "14:30"
export function timeOf(dateTime: string): string {
  return dateTime.slice(11, 16)
}

// "2026-11-06" → "Friday, 6 November 2026"
export function formatDayLong(dateKey: string): string {
  const [year, month, day] = dateKey.split('-').map(Number)
  const date = new Date(year, month - 1, day)
  const weekday = date.toLocaleDateString('en-GB', { weekday: 'long' })
  return `${weekday}, ${day} ${MONTHS[month - 1]} ${year}`
}

// (2026, 10) → "November 2026"   (month is 0-based, like Date)
export function formatMonth(year: number, month: number): string {
  return `${MONTHS[month]} ${year}`
}

// The days shown in a month view: whole weeks from Monday to Sunday,
// including the end of the previous month and the start of the next one.
export function monthGrid(year: number, month: number): Date[] {
  const first = new Date(year, month, 1)
  // getDay(): Sunday = 0 … Saturday = 6. We want Monday = 0 … Sunday = 6.
  const offset = (first.getDay() + 6) % 7
  const start = new Date(year, month, 1 - offset)

  const last = new Date(year, month + 1, 0)
  const daysShown = offset + last.getDate()
  const weeks = Math.ceil(daysShown / 7)

  const days: Date[] = []
  for (let i = 0; i < weeks * 7; i += 1) {
    days.push(new Date(start.getFullYear(), start.getMonth(), start.getDate() + i))
  }
  return days
}

import { useEffect, useMemo, useState } from 'react'
import Page from '../components/Page.tsx'
import EventForm from '../components/EventForm.tsx'
import controls from '../components/Controls.module.css'
import { createEvent, deleteEvent, getEvents, updateEvent, type EventInput } from '../api/events.ts'
import type { CalendarEvent } from '../types/models.ts'
import {
  WEEKDAYS,
  dateKeyOf,
  formatDayLong,
  formatMonth,
  monthGrid,
  timeOf,
  toDateKey,
} from '../utils/dates.ts'
import styles from './CalendarPage.module.css'

// How many event titles fit in one day cell before "+N more".
const CHIPS_PER_DAY = 2

// What the form below the calendar is doing: nothing, adding, or editing one event.
type Editing = { kind: 'none' } | { kind: 'new' } | { kind: 'edit'; event: CalendarEvent }

function timeLabel(event: CalendarEvent, dayKey: string): string {
  if (event.allDay) {
    return 'All day'
  }
  const startsToday = dateKeyOf(event.start) === dayKey
  const endsToday = dateKeyOf(event.end) === dayKey
  const from = startsToday ? timeOf(event.start) : '…'
  const to = endsToday ? timeOf(event.end) : '…'
  return `${from} – ${to}`
}

export default function CalendarPage() {
  // Read the clock once, when the page opens.
  const [todayKey] = useState(() => toDateKey(new Date()))
  const [view, setView] = useState(() => {
    const now = new Date()
    return { year: now.getFullYear(), month: now.getMonth() }
  })
  const [selectedDay, setSelectedDay] = useState(todayKey)
  const [events, setEvents] = useState<CalendarEvent[]>([])
  const [editing, setEditing] = useState<Editing>({ kind: 'none' })
  // Goes up after every change, so the events are loaded again.
  const [reloadCount, setReloadCount] = useState(0)

  const days = useMemo(() => monthGrid(view.year, view.month), [view])
  const firstKey = toDateKey(days[0])
  const lastKey = toDateKey(days[days.length - 1])

  // Load the events of the days on screen.
  // If the month changes before the answer arrives, the old answer is ignored.
  useEffect(() => {
    const request = new AbortController()
    void getEvents(firstKey, lastKey).then((loaded) => {
      if (!request.signal.aborted) {
        setEvents(loaded)
      }
    })
    return () => request.abort()
  }, [firstKey, lastKey, reloadCount])

  // Every day an event touches gets it, so multi-day events show on each of their days.
  function eventsOn(dayKey: string): CalendarEvent[] {
    return events.filter((e) => dateKeyOf(e.start) <= dayKey && dateKeyOf(e.end) >= dayKey)
  }

  function showMonth(delta: number) {
    const target = new Date(view.year, view.month + delta, 1)
    setView({ year: target.getFullYear(), month: target.getMonth() })
  }

  function goToToday() {
    const now = new Date()
    setView({ year: now.getFullYear(), month: now.getMonth() })
    setSelectedDay(toDateKey(now))
  }

  function selectDay(day: Date) {
    setSelectedDay(toDateKey(day))
    setEditing({ kind: 'none' })
    // Clicking a grey day from the next or previous month opens that month.
    if (day.getMonth() !== view.month) {
      setView({ year: day.getFullYear(), month: day.getMonth() })
    }
  }

  async function saveNew(input: EventInput) {
    await createEvent(input)
    setEditing({ kind: 'none' })
    setReloadCount((n) => n + 1)
  }

  async function saveEdit(eventId: string, input: EventInput) {
    await updateEvent(eventId, input)
    setEditing({ kind: 'none' })
    setReloadCount((n) => n + 1)
  }

  async function remove(eventId: string) {
    await deleteEvent(eventId)
    setEditing({ kind: 'none' })
    setReloadCount((n) => n + 1)
  }

  const selectedEvents = eventsOn(selectedDay)

  return (
    <Page title="Calendar">
      <div className={styles.toolbar}>
        <h2 className={styles.month}>{formatMonth(view.year, view.month)}</h2>
        <div className={controls.actions}>
          <button type="button" className={`${controls.button} ${controls.small}`} onClick={goToToday}>
            Today
          </button>
          <button
            type="button"
            className={`${controls.button} ${controls.small}`}
            onClick={() => showMonth(-1)}
            aria-label="Previous month"
          >
            ←
          </button>
          <button
            type="button"
            className={`${controls.button} ${controls.small}`}
            onClick={() => showMonth(1)}
            aria-label="Next month"
          >
            →
          </button>
        </div>
      </div>

      <div className={styles.calendar}>
        <div className={styles.weekdays} aria-hidden="true">
          {WEEKDAYS.map((name) => (
            <span key={name}>{name}</span>
          ))}
        </div>

        <div className={styles.grid}>
          {days.map((day) => {
            const key = toDateKey(day)
            const dayEvents = eventsOn(key)
            const hidden = dayEvents.length - CHIPS_PER_DAY
            const classes = [
              styles.day,
              day.getMonth() !== view.month ? styles.otherMonth : '',
              key === todayKey ? styles.today : '',
              key === selectedDay ? styles.selected : '',
            ]

            return (
              <button
                key={key}
                type="button"
                className={classes.filter(Boolean).join(' ')}
                onClick={() => selectDay(day)}
                aria-pressed={key === selectedDay}
                aria-label={`${formatDayLong(key)}, ${dayEvents.length} ${dayEvents.length === 1 ? 'event' : 'events'}`}
              >
                <span className={styles.dayNumber}>{day.getDate()}</span>
                {dayEvents.slice(0, CHIPS_PER_DAY).map((event) => (
                  <span key={event.id} className={styles.chip}>
                    {event.title}
                  </span>
                ))}
                {hidden > 0 && <span className={styles.more}>+{hidden} more</span>}
              </button>
            )
          })}
        </div>
      </div>

      <section className={styles.dayPanel} aria-live="polite">
        <div className={styles.dayHeader}>
          <h2 className={styles.dayTitle}>{formatDayLong(selectedDay)}</h2>
          {editing.kind === 'none' && (
            <button
              type="button"
              className={`${controls.button} ${controls.primary}`}
              onClick={() => setEditing({ kind: 'new' })}
            >
              + Add event
            </button>
          )}
        </div>

        {editing.kind === 'new' && (
          <EventForm
            event={null}
            defaultDate={selectedDay}
            onSave={saveNew}
            onCancel={() => setEditing({ kind: 'none' })}
          />
        )}

        {editing.kind === 'edit' && (
          <EventForm
            key={editing.event.id}
            event={editing.event}
            defaultDate={selectedDay}
            onSave={(input) => saveEdit(editing.event.id, input)}
            onDelete={() => remove(editing.event.id)}
            onCancel={() => setEditing({ kind: 'none' })}
          />
        )}

        {selectedEvents.length === 0 ? (
          <p className={controls.hint}>No events on this day.</p>
        ) : (
          <ul className={styles.events}>
            {selectedEvents.map((event) => (
              <li key={event.id} className={styles.event}>
                <span className={styles.eventTime}>{timeLabel(event, selectedDay)}</span>
                <span className={styles.eventBody}>
                  <span className={styles.eventTitle}>{event.title}</span>
                  {event.description && <span className={styles.eventDescription}>{event.description}</span>}
                </span>
                <button
                  type="button"
                  className={`${controls.button} ${controls.small}`}
                  onClick={() => setEditing({ kind: 'edit', event })}
                  aria-label={`Edit ${event.title}`}
                >
                  Edit
                </button>
              </li>
            ))}
          </ul>
        )}

        <p className={controls.hint}>For now events are kept only until you refresh the page.</p>
      </section>
    </Page>
  )
}

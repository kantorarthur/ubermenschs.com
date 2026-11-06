import { useState, type FormEvent } from 'react'
import { ALL_DAY_NOTIFY_OPTIONS, EVENT_LIMITS, NOTIFY_OPTIONS, type EventInput } from '../api/events.ts'
import type { CalendarEvent } from '../types/models.ts'
import { dateKeyOf, timeOf } from '../utils/dates.ts'
import controls from './Controls.module.css'
import styles from './EventForm.module.css'

interface EventFormProps {
  // The event being edited, or null for a new one.
  event: CalendarEvent | null
  // The day a new event starts on ("YYYY-MM-DD").
  defaultDate: string
  onSave: (input: EventInput) => Promise<void>
  onDelete?: () => Promise<void>
  onCancel: () => void
}

// Add or edit one calendar event.
export default function EventForm({ event, defaultDate, onSave, onDelete, onCancel }: EventFormProps) {
  const [title, setTitle] = useState(event?.title ?? '')
  const [allDay, setAllDay] = useState(event?.allDay ?? false)
  const [startDate, setStartDate] = useState(event ? dateKeyOf(event.start) : defaultDate)
  const [endDate, setEndDate] = useState(event ? dateKeyOf(event.end) : defaultDate)
  const [startTime, setStartTime] = useState(event && !event.allDay ? timeOf(event.start) : '09:00')
  const [endTime, setEndTime] = useState(event && !event.allDay ? timeOf(event.end) : '10:00')
  const [description, setDescription] = useState(event?.description ?? '')
  const [notify, setNotify] = useState<number[]>(event?.notify ?? [])

  const notifyOptions = allDay ? ALL_DAY_NOTIFY_OPTIONS : NOTIFY_OPTIONS

  function toggleNotify(minutes: number, on: boolean) {
    setNotify((current) => (on ? [...current, minutes] : current.filter((m) => m !== minutes)))
  }

  function changeAllDay(value: boolean) {
    setAllDay(value)
    // Keep only the choices the new kind of event offers.
    const offered = (value ? ALL_DAY_NOTIFY_OPTIONS : NOTIFY_OPTIONS).map((option) => option.minutes)
    setNotify((current) => current.filter((m) => offered.includes(m)))
  }

  const [error, setError] = useState<string | null>(null)
  const [isSaving, setIsSaving] = useState(false)

  function changeStartDate(value: string) {
    setStartDate(value)
    // Keep the end on or after the start.
    if (endDate < value) {
      setEndDate(value)
    }
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    setError(null)
    setIsSaving(true)
    try {
      await onSave({
        title,
        allDay,
        start: `${startDate}T${allDay ? '00:00' : startTime}`,
        end: `${endDate}T${allDay ? '23:59' : endTime}`,
        description,
        notify: [...notify].sort((a, b) => a - b),
      })
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Something went wrong.')
      setIsSaving(false)
    }
  }

  async function handleDelete() {
    if (!onDelete || !window.confirm(`Delete the event “${event?.title}”?`)) {
      return
    }
    setIsSaving(true)
    try {
      await onDelete()
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Something went wrong.')
      setIsSaving(false)
    }
  }

  return (
    <form className={controls.form} onSubmit={handleSubmit}>
      <h2 className={styles.heading}>{event ? 'Edit event' : 'New event'}</h2>

      <label className={controls.field}>
        Title
        <input
          className={controls.input}
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          maxLength={EVENT_LIMITS.title}
          autoFocus
        />
      </label>

      <label className={styles.checkbox}>
        <input type="checkbox" checked={allDay} onChange={(e) => changeAllDay(e.target.checked)} />
        All day
      </label>

      <div className={styles.row}>
        <label className={controls.field}>
          Start date
          <input
            type="date"
            className={controls.input}
            value={startDate}
            onChange={(e) => changeStartDate(e.target.value)}
            required
          />
        </label>
        {!allDay && (
          <label className={controls.field}>
            Start time
            <input
              type="time"
              className={controls.input}
              value={startTime}
              onChange={(e) => setStartTime(e.target.value)}
              required
            />
          </label>
        )}
      </div>

      <div className={styles.row}>
        <label className={controls.field}>
          End date
          <input
            type="date"
            className={controls.input}
            value={endDate}
            min={startDate}
            onChange={(e) => setEndDate(e.target.value)}
            required
          />
        </label>
        {!allDay && (
          <label className={controls.field}>
            End time
            <input
              type="time"
              className={controls.input}
              value={endTime}
              onChange={(e) => setEndTime(e.target.value)}
              required
            />
          </label>
        )}
      </div>

      <label className={controls.field}>
        Description (optional)
        <textarea
          className={controls.input}
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          maxLength={EVENT_LIMITS.description}
        />
      </label>

      <fieldset className={styles.notify}>
        <legend className={styles.legend}>Notify me</legend>
        {notifyOptions.map((option) => (
          <label key={option.minutes} className={styles.checkbox}>
            <input
              type="checkbox"
              checked={notify.includes(option.minutes)}
              onChange={(e) => toggleNotify(option.minutes, e.target.checked)}
            />
            {option.label}
          </label>
        ))}
        <p className={controls.hint}>
          Notifications go to the devices where they are turned on (on the Reminder page).
        </p>
      </fieldset>

      {error && <p className={controls.error}>{error}</p>}

      <div className={controls.actions}>
        <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isSaving}>
          {event ? 'Save changes' : 'Add event'}
        </button>
        <button type="button" className={controls.button} onClick={onCancel}>
          Cancel
        </button>
        {onDelete && (
          <button
            type="button"
            className={`${controls.button} ${controls.danger}`}
            onClick={handleDelete}
            disabled={isSaving}
          >
            Delete event
          </button>
        )}
      </div>
    </form>
  )
}

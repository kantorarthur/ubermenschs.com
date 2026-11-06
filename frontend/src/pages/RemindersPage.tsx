import { useEffect, useState, type FormEvent } from 'react'
import Page from '../components/Page.tsx'
import LoginPrompt from '../components/LoginPrompt.tsx'
import NotificationSettings from '../components/NotificationSettings.tsx'
import controls from '../components/Controls.module.css'
import {
  REMINDER_LIMITS,
  browserTimeZone,
  createReminder,
  deleteReminder,
  getReminders,
  updateReminder,
} from '../api/reminders.ts'
import { useAuth } from '../context/authContext.ts'
import type { Reminder } from '../types/models.ts'
import styles from './RemindersPage.module.css'

function errorMessage(e: unknown): string {
  return e instanceof Error ? e.message : 'Something went wrong.'
}

const SUBTITLE =
  "Messages delivered as a notification every day at the time you choose. Turn one off when you don't want it."

export default function RemindersPage() {
  const { user, isLoading } = useAuth()

  if (isLoading) {
    return <Page title="Reminder" subtitle={SUBTITLE} />
  }
  if (!user) {
    return (
      <Page title="Reminder" subtitle={SUBTITLE}>
        <LoginPrompt>Log in to set your daily reminders.</LoginPrompt>
      </Page>
    )
  }
  // key: another user starts with a fresh list.
  return <ReminderList key={user.id} />
}

function ReminderList() {
  const [reminders, setReminders] = useState<Reminder[] | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [isBusy, setIsBusy] = useState(false)

  // The "new reminder" form.
  const [time, setTime] = useState('07:00')
  const [message, setMessage] = useState('')

  // The reminder being edited in place, if any.
  const [editingId, setEditingId] = useState<string | null>(null)
  const [editTime, setEditTime] = useState('')
  const [editMessage, setEditMessage] = useState('')

  const [zone] = useState(browserTimeZone)

  useEffect(() => {
    let cancelled = false
    getReminders()
      .then((loaded) => {
        if (!cancelled) {
          setReminders(loaded)
        }
      })
      .catch((e: unknown) => {
        if (!cancelled) {
          setError(errorMessage(e))
        }
      })
    return () => {
      cancelled = true
    }
  }, [])

  // Runs a change, then reloads the list.
  async function run(change: () => Promise<unknown>): Promise<boolean> {
    setError(null)
    setIsBusy(true)
    try {
      await change()
      setReminders(await getReminders())
      return true
    } catch (e) {
      setError(errorMessage(e))
      return false
    } finally {
      setIsBusy(false)
    }
  }

  async function handleCreate(event: FormEvent) {
    event.preventDefault()
    const ok = await run(() => createReminder({ time, message }))
    if (ok) {
      setMessage('')
    }
  }

  function startEditing(reminder: Reminder) {
    setEditingId(reminder.id)
    setEditTime(reminder.time)
    setEditMessage(reminder.message)
    setError(null)
  }

  async function handleSaveEdit(event: FormEvent) {
    event.preventDefault()
    if (!editingId) {
      return
    }
    const ok = await run(() => updateReminder(editingId, { time: editTime, message: editMessage }))
    if (ok) {
      setEditingId(null)
    }
  }

  async function handleDelete(reminder: Reminder) {
    if (!window.confirm(`Delete the ${reminder.time} reminder?`)) {
      return
    }
    await run(() => deleteReminder(reminder.id))
  }

  return (
    <Page title="Reminder" subtitle={SUBTITLE}>
      <NotificationSettings />

      <form className={controls.form} onSubmit={handleCreate}>
        <div className={styles.newRow}>
          <label className={controls.field}>
            Time
            <input
              type="time"
              className={`${controls.input} ${styles.timeInput}`}
              value={time}
              onChange={(e) => setTime(e.target.value)}
              required
            />
          </label>
          <label className={`${controls.field} ${styles.grow}`}>
            Message
            <input
              className={controls.input}
              value={message}
              onChange={(e) => setMessage(e.target.value)}
              maxLength={REMINDER_LIMITS.message}
              placeholder="e.g. Read 20 pages before anything else."
            />
          </label>
        </div>
        <div className={controls.actions}>
          <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isBusy}>
            Add reminder
          </button>
        </div>
      </form>

      {error && <p className={controls.error}>{error}</p>}

      {reminders !== null && reminders.length === 0 && (
        <p className={controls.hint}>No reminders yet. Add the first one above.</p>
      )}

      {reminders !== null && reminders.length > 0 && (
        <ul className={styles.list}>
          {reminders.map((reminder) =>
            reminder.id === editingId ? (
              <li key={reminder.id}>
                <form className={controls.form} onSubmit={handleSaveEdit}>
                  <div className={styles.newRow}>
                    <label className={controls.field}>
                      Time
                      <input
                        type="time"
                        className={`${controls.input} ${styles.timeInput}`}
                        value={editTime}
                        onChange={(e) => setEditTime(e.target.value)}
                        required
                      />
                    </label>
                    <label className={`${controls.field} ${styles.grow}`}>
                      Message
                      <input
                        className={controls.input}
                        value={editMessage}
                        onChange={(e) => setEditMessage(e.target.value)}
                        maxLength={REMINDER_LIMITS.message}
                        autoFocus
                      />
                    </label>
                  </div>
                  <div className={controls.actions}>
                    <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isBusy}>
                      Save
                    </button>
                    <button type="button" className={controls.button} onClick={() => setEditingId(null)}>
                      Cancel
                    </button>
                  </div>
                </form>
              </li>
            ) : (
              <li key={reminder.id} className={reminder.enabled ? styles.reminder : `${styles.reminder} ${styles.off}`}>
                <span className={styles.time}>{reminder.time}</span>
                <span className={styles.body}>
                  <span className={styles.message}>{reminder.message}</span>
                  <span className={styles.status}>
                    {reminder.enabled ? 'Every day' : 'Off'}
                    {/* Set from another time zone (e.g. while travelling): say which one. */}
                    {reminder.timezone !== zone && ` · ${reminder.timezone} time`}
                  </span>
                </span>
                <span className={styles.controls}>
                  {/* A switch: a real button that screen readers announce as on/off. */}
                  <button
                    type="button"
                    role="switch"
                    aria-checked={reminder.enabled}
                    aria-label={`Reminder at ${reminder.time}`}
                    className={styles.switch}
                    onClick={() => void run(() => updateReminder(reminder.id, { enabled: !reminder.enabled }))}
                    disabled={isBusy}
                  >
                    <span className={styles.knob} />
                  </button>
                  <button
                    type="button"
                    className={`${controls.button} ${controls.small}`}
                    onClick={() => startEditing(reminder)}
                  >
                    Edit
                  </button>
                  <button
                    type="button"
                    className={`${controls.button} ${controls.small} ${controls.danger}`}
                    onClick={() => void handleDelete(reminder)}
                    disabled={isBusy}
                    aria-label={`Delete the ${reminder.time} reminder`}
                  >
                    Delete
                  </button>
                </span>
              </li>
            ),
          )}
        </ul>
      )}

      <p className={controls.hint}>
        Times are in your time zone ({zone}). A notification goes to every device where notifications are on.
      </p>
    </Page>
  )
}

import { useEffect, useState } from 'react'
import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import controls from '../components/Controls.module.css'
import { getEvents } from '../api/events.ts'
import { getReminders } from '../api/reminders.ts'
import { useAuth } from '../context/authContext.ts'
import type { CalendarEvent, Reminder } from '../types/models.ts'
import { dateKeyOf, formatDayLong, timeOf, toDateKey } from '../utils/dates.ts'
import styles from './OverviewPage.module.css'

const SECTIONS = [
  { to: '/quotes', title: 'Set quotes', text: 'Choose which quotes appear at the top of every page, or make your own sets.' },
  { to: '/stories', title: 'Philosophical stories', text: 'Short stories from the lives and ideas of the philosophers.' },
  {
    to: '/love-one-another',
    title: 'Love each other as I have loved you.',
    text: 'Passages from the Bible to come back to.',
  },
  { to: '/calendar', title: 'Calendar', text: 'Your events, with a notification before they start.' },
  { to: '/reminders', title: 'Reminder', text: 'Daily messages at the time you choose, on your phone and computer.' },
  { to: '/study-plan', title: 'Philosophical study plan', text: 'What to read and in which order. Coming soon.' },
]

// "/" — the main page: what the site is, today for a logged-in user, and the way to every section.
export default function OverviewPage() {
  const { user, isLoading } = useAuth()

  return (
    <Page title="Overview" subtitle="One step closer to the Übermensch than you were yesterday.">
      <p className={styles.intro}>
        ubermenschs.com is a place for a daily practice of becoming better: a philosophical quote on every page, stories
        and passages worth rereading, your calendar and your daily reminders.
      </p>

      {!isLoading && (user ? <Today key={user.id} /> : <Welcome />)}

      <h2 className={styles.heading}>Sections</h2>
      <ul className={styles.sections}>
        {SECTIONS.map((section) => (
          <li key={section.to}>
            <Link to={section.to} className={styles.section}>
              <span className={styles.sectionTitle}>{section.title}</span>
              <span className={styles.sectionText}>{section.text}</span>
            </Link>
          </li>
        ))}
      </ul>
    </Page>
  )
}

function Welcome() {
  return (
    <div className={styles.card}>
      <p>Make an account to keep your own quote sets, your calendar and your daily reminders.</p>
      <div className={controls.actions}>
        <Link to="/register" className={`${controls.button} ${controls.primary}`}>
          Register
        </Link>
        <Link to="/login" className={controls.button}>
          Log in
        </Link>
      </div>
    </div>
  )
}

// Seven days from today, for "Coming up".
function inDays(days: number): string {
  const date = new Date()
  date.setDate(date.getDate() + days)
  return toDateKey(date)
}

function Today() {
  const [todayKey] = useState(() => toDateKey(new Date()))
  const [now] = useState(() => {
    const date = new Date()
    return `${String(date.getHours()).padStart(2, '0')}:${String(date.getMinutes()).padStart(2, '0')}`
  })
  const [events, setEvents] = useState<CalendarEvent[] | null>(null)
  const [reminders, setReminders] = useState<Reminder[] | null>(null)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    let cancelled = false
    Promise.all([getEvents(todayKey, inDays(7)), getReminders()])
      .then(([loadedEvents, loadedReminders]) => {
        if (!cancelled) {
          setEvents(loadedEvents)
          setReminders(loadedReminders)
        }
      })
      .catch((e: unknown) => {
        if (!cancelled) {
          setError(e instanceof Error ? e.message : 'Today could not be loaded.')
        }
      })
    return () => {
      cancelled = true
    }
  }, [todayKey])

  if (error) {
    return <p className={controls.error}>{error}</p>
  }
  if (!events || !reminders) {
    return <p className={controls.hint}>Loading today…</p>
  }

  const today = events.filter((e) => dateKeyOf(e.start) <= todayKey && dateKeyOf(e.end) >= todayKey)
  const comingUp = events.filter((e) => dateKeyOf(e.start) > todayKey).slice(0, 5)
  const activeReminders = reminders.filter((r) => r.enabled)

  return (
    <div className={styles.today}>
      <section className={styles.card} aria-labelledby="today-heading">
        <h2 id="today-heading" className={styles.cardTitle}>
          Today · {formatDayLong(todayKey)}
        </h2>
        {today.length === 0 ? (
          <p className={controls.hint}>Nothing in the calendar today.</p>
        ) : (
          <ul className={styles.list}>
            {today.map((event) => (
              <li key={event.id}>
                <span className={styles.time}>{event.allDay ? 'All day' : timeOf(event.start)}</span>
                {event.title}
              </li>
            ))}
          </ul>
        )}

        {comingUp.length > 0 && (
          <>
            <h3 className={styles.subTitle}>Coming up</h3>
            <ul className={styles.list}>
              {comingUp.map((event) => (
                <li key={event.id}>
                  <span className={styles.time}>
                    {formatDayLong(dateKeyOf(event.start)).split(',')[0]}
                    {!event.allDay && ` ${timeOf(event.start)}`}
                  </span>
                  {event.title}
                </li>
              ))}
            </ul>
          </>
        )}
        <Link to="/calendar" className={styles.more}>
          Open the calendar →
        </Link>
      </section>

      <section className={styles.card} aria-labelledby="reminders-heading">
        <h2 id="reminders-heading" className={styles.cardTitle}>
          Reminders today
        </h2>
        {activeReminders.length === 0 ? (
          <p className={controls.hint}>No reminders are on.</p>
        ) : (
          <ul className={styles.list}>
            {activeReminders.map((reminder) => (
              <li key={reminder.id} className={reminder.time <= now ? styles.done : undefined}>
                <span className={styles.time}>{reminder.time}</span>
                {reminder.message}
              </li>
            ))}
          </ul>
        )}
        <Link to="/reminders" className={styles.more}>
          Open the reminders →
        </Link>
      </section>
    </div>
  )
}

import { useEffect, useState } from 'react'
import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import LoginPrompt from '../components/LoginPrompt.tsx'
import controls from '../components/Controls.module.css'
import { createReminder, getReminders } from '../api/reminders.ts'
import { getStudyProgress, setBookRead } from '../api/study.ts'
import { useAuth } from '../context/authContext.ts'
import { ALL_BOOKS, ALONG_THE_WAY, STUDY_PLAN, nextBook } from '../data/studyPlan.ts'
import styles from './StudyPlanPage.module.css'

const SUBTITLE = 'From the Stoics to the Übermensch, in five stages. One book at a time.'
const REMINDER_TIME = '21:00'
// One reminder for the whole plan: it stays right when "Reading now" moves to the next book.
const READING_REMINDER = 'Read 20 minutes of your philosophy book.'

function errorMessage(e: unknown): string {
  return e instanceof Error ? e.message : 'Something went wrong.'
}

// "/study-plan": the reading plan (src/data/studyPlan.ts). Visitors see the plan; a logged-in user
// also ticks the books they read, sees which one is next, and can get a daily reminder to read.
export default function StudyPlanPage() {
  const { user, isLoading } = useAuth()

  return (
    <Page title="Philosophical study plan" subtitle={SUBTITLE}>
      {!isLoading &&
        (user ? (
          // key: another user starts with their own progress.
          <TrackedPlan key={user.id} />
        ) : (
          <>
            <LoginPrompt>Log in to tick the books you read and keep your progress.</LoginPrompt>
            <Stages read={null} />
          </>
        ))}
      <AlongTheWay />
    </Page>
  )
}

function TrackedPlan() {
  const [read, setRead] = useState<Set<string> | null>(null)
  const [reminderMessages, setReminderMessages] = useState<Set<string>>(new Set())
  const [error, setError] = useState<string | null>(null)
  const [savingBookId, setSavingBookId] = useState<string | null>(null)
  const [isAddingReminder, setIsAddingReminder] = useState(false)
  const [addedReminder, setAddedReminder] = useState(false)

  useEffect(() => {
    let cancelled = false
    getStudyProgress()
      .then((progress) => {
        if (!cancelled) {
          setRead(new Set(progress.read))
        }
      })
      .catch((e: unknown) => {
        if (!cancelled) {
          setError(errorMessage(e))
        }
      })
    // Only to know whether a reading reminder exists already; the plan works without it.
    getReminders()
      .then((reminders) => {
        if (!cancelled) {
          setReminderMessages(new Set(reminders.map((r) => r.message)))
        }
      })
      .catch(() => undefined)
    return () => {
      cancelled = true
    }
  }, [])

  // The tick changes at once; if saving fails, it goes back and the error is shown.
  function withBook(books: Set<string> | null, bookId: string, isRead: boolean): Set<string> {
    const changed = new Set(books)
    if (isRead) {
      changed.add(bookId)
    } else {
      changed.delete(bookId)
    }
    return changed
  }

  async function toggle(bookId: string, isRead: boolean) {
    setError(null)
    setSavingBookId(bookId)
    setRead((books) => withBook(books, bookId, isRead))
    try {
      const progress = await setBookRead(bookId, isRead)
      setRead(new Set(progress.read))
    } catch (e) {
      setRead((books) => withBook(books, bookId, !isRead))
      setError(errorMessage(e))
    } finally {
      setSavingBookId(null)
    }
  }

  async function addReminder() {
    setError(null)
    setIsAddingReminder(true)
    try {
      const reminder = await createReminder({ time: REMINDER_TIME, message: READING_REMINDER })
      setReminderMessages((messages) => new Set(messages).add(reminder.message))
      setAddedReminder(true)
    } catch (e) {
      setError(errorMessage(e))
    } finally {
      setIsAddingReminder(false)
    }
  }

  if (!read) {
    return error ? <p className={controls.error}>{error}</p> : <p className={controls.hint}>Loading your progress…</p>
  }

  const next = nextBook(read)
  const readCount = ALL_BOOKS.filter(({ book }) => read.has(book.id)).length
  const percent = Math.round((readCount / ALL_BOOKS.length) * 100)
  const hasReminder = reminderMessages.has(READING_REMINDER)

  return (
    <>
      {next ? (
        <section className={`${styles.panel} ${styles.now}`} aria-labelledby="reading-now">
          <div>
            <p className={styles.eyebrow}>Reading now · Stage {next.stageNumber}</p>
            <h2 id="reading-now" className={styles.nowTitle}>
              {next.book.author} — {next.book.title}
            </h2>
            <p className={styles.muted}>
              {next.book.about} {next.book.duration.replace('~', 'About ')}.
            </p>
            <p className={styles.question}>“{next.book.question}”</p>
          </div>
          <div className={styles.nowButtons}>
            <button
              type="button"
              className={`${controls.button} ${controls.primary}`}
              onClick={() => void toggle(next.book.id, true)}
              disabled={savingBookId !== null}
            >
              Mark as read
            </button>
            <button
              type="button"
              className={controls.button}
              onClick={() => void addReminder()}
              disabled={hasReminder || isAddingReminder}
            >
              {hasReminder ? '✓ Daily reminder set' : `Remind me at ${REMINDER_TIME}`}
            </button>
          </div>
          {addedReminder && hasReminder && (
            <p className={`${controls.hint} ${styles.wide}`} role="status">
              A daily reminder at {REMINDER_TIME} was added. Change its time, or turn on notifications on this device, in{' '}
              <Link to="/reminders" className={styles.link}>
                Reminder
              </Link>
              .
            </p>
          )}
        </section>
      ) : (
        <section className={`${styles.panel} ${styles.now}`} aria-labelledby="reading-now">
          <div>
            <p className={styles.eyebrow}>The plan is finished</p>
            <h2 id="reading-now" className={styles.nowTitle}>
              Fifteen books, from Marcus Aurelius to Zarathustra.
            </h2>
            <p className={styles.question}>“Become who you are.”</p>
          </div>
        </section>
      )}

      {error && <p className={controls.error}>{error}</p>}

      <section className={styles.panel} aria-label="Your progress">
        <div className={styles.overall}>
          <span className={styles.eyebrow}>Your progress</span>
          <span className={styles.count}>
            {readCount} of {ALL_BOOKS.length} books
            {next ? ` · Stage ${next.stageNumber} of ${STUDY_PLAN.length}` : ' · finished'}
          </span>
        </div>
        <div
          className={styles.bar}
          role="progressbar"
          aria-label="Books read"
          aria-valuemin={0}
          aria-valuemax={ALL_BOOKS.length}
          aria-valuenow={readCount}
        >
          <span style={{ width: `${percent}%` }} />
        </div>
      </section>

      <Stages
        read={read}
        currentStageId={next?.stage.id}
        currentBookId={next?.book.id}
        savingBookId={savingBookId}
        onToggle={(bookId, isRead) => void toggle(bookId, isRead)}
      />
    </>
  )
}

interface StagesProps {
  // null: a visitor, who sees the plan without the ticks.
  read: Set<string> | null
  currentStageId?: string
  currentBookId?: string
  savingBookId?: string | null
  onToggle?: (bookId: string, read: boolean) => void
}

function Stages({ read, currentStageId, currentBookId, savingBookId, onToggle }: StagesProps) {
  // Only the stages the user opened or closed themselves; the others follow the current stage.
  const [openStages, setOpenStages] = useState<Record<string, boolean>>({})
  const defaultOpen = currentStageId ?? STUDY_PLAN[0].id

  return (
    <div className={styles.stages}>
      {STUDY_PLAN.map((stage, index) => {
        const isOpen = openStages[stage.id] ?? stage.id === defaultOpen
        const readInStage = read ? stage.books.filter((b) => read.has(b.id)).length : 0
        const contentId = `stage-${stage.id}`

        return (
          <section key={stage.id} className={styles.panel} aria-labelledby={`${contentId}-title`}>
            <button
              type="button"
              className={styles.stageHead}
              aria-expanded={isOpen}
              aria-controls={contentId}
              onClick={() => setOpenStages((open) => ({ ...open, [stage.id]: !isOpen }))}
            >
              <span className={styles.number} aria-hidden="true">
                {index + 1}
              </span>
              <span className={styles.stageText}>
                <span id={`${contentId}-title`} className={styles.stageTitle}>
                  {stage.title}
                </span>
                <span className={styles.muted}>
                  {stage.subtitle} · {stage.duration}
                </span>
              </span>
              {read && (
                <span className={styles.count}>
                  {readInStage} / {stage.books.length}
                </span>
              )}
              <span className={styles.chevron} aria-hidden="true">
                {isOpen ? '▴' : '▾'}
              </span>
            </button>

            {isOpen && (
              <div id={contentId} className={styles.stageBody}>
                <ul className={styles.books}>
                  {stage.books.map((book) => {
                    const isRead = read?.has(book.id) ?? false
                    return (
                      <li key={book.id} className={styles.book}>
                        {read ? (
                          <input
                            type="checkbox"
                            className={styles.check}
                            checked={isRead}
                            disabled={savingBookId === book.id}
                            onChange={(e) => {
                              // The stage being ticked stays open, also when its last book moves
                              // "Reading now" to the next stage.
                              setOpenStages((open) => ({ ...open, [stage.id]: true }))
                              onToggle?.(book.id, e.target.checked)
                            }}
                            aria-label={`${book.title} read`}
                          />
                        ) : (
                          <span className={styles.dot} aria-hidden="true" />
                        )}
                        <div className={isRead ? styles.doneText : undefined}>
                          <div className={styles.bookTitle}>{book.title}</div>
                          <div className={styles.muted}>
                            {book.author} · {book.about}
                          </div>
                        </div>
                        <span className={book.id === currentBookId ? `${styles.tag} ${styles.reading}` : styles.tag}>
                          {isRead ? 'Read' : book.id === currentBookId ? 'Reading now' : book.duration}
                        </span>
                      </li>
                    )
                  })}
                </ul>
                <p className={styles.exercise}>
                  <strong>Exercise · {stage.exercise.name}:</strong> {stage.exercise.text}
                </p>
              </div>
            )}
          </section>
        )
      })}
    </div>
  )
}

function AlongTheWay() {
  return (
    <section className={styles.panel} aria-labelledby="along-the-way">
      <h2 id="along-the-way" className={styles.stageTitle}>
        Along the way
      </h2>
      <p className={styles.muted}>Not part of the count: to read beside the plan, or after it.</p>
      <ul className={styles.books}>
        {ALONG_THE_WAY.map((book) => (
          <li key={book.title} className={styles.extra}>
            <div className={styles.bookTitle}>{book.title}</div>
            <div className={styles.muted}>
              {book.author} · {book.about}
            </div>
          </li>
        ))}
      </ul>
    </section>
  )
}

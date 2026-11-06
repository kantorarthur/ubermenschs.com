import { useEffect, useState } from 'react'
import { sendTestNotification } from '../api/push.ts'
import {
  notificationState,
  syncSubscription,
  turnOffNotifications,
  turnOnNotifications,
  type NotificationState,
} from '../utils/notifications.ts'
import controls from './Controls.module.css'
import styles from './NotificationSettings.module.css'

function errorMessage(e: unknown): string {
  return e instanceof Error ? e.message : 'Something went wrong.'
}

const DESCRIPTIONS: Record<NotificationState, string> = {
  unsupported:
    'This browser cannot show notifications from websites. On iPhone or iPad, add the site to the home screen ' +
    '(Share → Add to Home Screen), open it from there and come back to this page.',
  denied: 'Notifications are blocked for this site. Allow them in the browser’s site settings, then reload the page.',
  off: 'Turn them on to get your reminders on this device, even when the site is closed.',
  on: 'This device gets your reminders.',
}

// The "Notifications on this device" panel on the Reminder page.
export default function NotificationSettings() {
  // null while it is being checked
  const [state, setState] = useState<NotificationState | null>(null)
  const [isBusy, setIsBusy] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [info, setInfo] = useState<string | null>(null)

  useEffect(() => {
    let cancelled = false
    notificationState()
      .then(async (current) => {
        // Make sure the backend links this browser to the account that is logged in now.
        if (current === 'on') {
          await syncSubscription()
        }
        if (!cancelled) {
          setState(current)
        }
      })
      .catch((e: unknown) => {
        if (!cancelled) {
          setError(errorMessage(e))
          setState('off')
        }
      })
    return () => {
      cancelled = true
    }
  }, [])

  async function run(action: () => Promise<void>) {
    setError(null)
    setInfo(null)
    setIsBusy(true)
    try {
      await action()
    } catch (e) {
      setError(errorMessage(e))
    } finally {
      setState(await notificationState().catch(() => 'off' as const))
      setIsBusy(false)
    }
  }

  async function test() {
    const { sent, devices } = await sendTestNotification()
    if (devices === 0) {
      setInfo('No device has notifications on yet.')
    } else if (sent === devices) {
      setInfo(sent === 1 ? 'Sent. It should appear in a few seconds.' : `Sent to your ${sent} devices.`)
    } else {
      setInfo(`Sent to ${sent} of your ${devices} devices; the others did not answer.`)
    }
  }

  return (
    <section className={styles.panel} aria-labelledby="notifications-heading">
      <div className={styles.text}>
        <h2 id="notifications-heading" className={styles.heading}>
          Notifications on this device
        </h2>
        <p className={controls.hint}>{state === null ? 'Checking…' : DESCRIPTIONS[state]}</p>
        {info && <p className={styles.info}>{info}</p>}
        {error && <p className={controls.error}>{error}</p>}
      </div>

      <div className={controls.actions}>
        {state === 'off' && (
          <button
            type="button"
            className={`${controls.button} ${controls.primary}`}
            onClick={() => void run(turnOnNotifications)}
            disabled={isBusy}
          >
            Turn on notifications
          </button>
        )}
        {state === 'on' && (
          <>
            <button type="button" className={controls.button} onClick={() => void run(test)} disabled={isBusy}>
              Send a test notification
            </button>
            <button
              type="button"
              className={`${controls.button} ${controls.small}`}
              onClick={() => void run(turnOffNotifications)}
              disabled={isBusy}
            >
              Turn off
            </button>
          </>
        )}
      </div>
    </section>
  )
}

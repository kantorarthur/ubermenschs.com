import { useState, type FormEvent } from 'react'
import Page from '../components/Page.tsx'
import LoginPrompt from '../components/LoginPrompt.tsx'
import controls from '../components/Controls.module.css'
import { PASSWORD_MIN, changePassword } from '../api/auth.ts'
import { useAuth } from '../context/authContext.ts'
import styles from './AccountPage.module.css'

// /account/settings: changing the password.
export default function AccountSettingsPage() {
  const { user, isLoading } = useAuth()

  if (isLoading) {
    return <Page title="Account settings" />
  }
  if (!user) {
    return (
      <Page title="Account settings">
        <LoginPrompt>Log in to change your account settings.</LoginPrompt>
      </Page>
    )
  }

  return (
    <Page title="Account settings">
      <ChangePasswordForm />
    </Page>
  )
}

function ChangePasswordForm() {
  const [currentPassword, setCurrentPassword] = useState('')
  const [newPassword, setNewPassword] = useState('')
  const [confirmation, setConfirmation] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [isDone, setIsDone] = useState(false)
  const [isSaving, setIsSaving] = useState(false)

  async function handleSubmit(event: FormEvent) {
    event.preventDefault()
    setError(null)
    setIsDone(false)

    // Checked here because only the browser sees both fields; the backend checks the rest.
    if (newPassword !== confirmation) {
      setError('The two new passwords are different.')
      return
    }
    if (newPassword === currentPassword) {
      setError('The new password is the same as the current one.')
      return
    }

    setIsSaving(true)
    try {
      await changePassword({ currentPassword, newPassword })
      setCurrentPassword('')
      setNewPassword('')
      setConfirmation('')
      setIsDone(true)
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Something went wrong.')
    } finally {
      setIsSaving(false)
    }
  }

  return (
    <form className={`${controls.form} ${styles.form}`} onSubmit={handleSubmit}>
      <h2 className={styles.formTitle}>Change password</h2>

      <label className={controls.field}>
        Current password
        <input
          type="password"
          className={controls.input}
          value={currentPassword}
          onChange={(e) => setCurrentPassword(e.target.value)}
          autoComplete="current-password"
          required
        />
      </label>
      <label className={controls.field}>
        New password
        <input
          type="password"
          className={controls.input}
          value={newPassword}
          onChange={(e) => setNewPassword(e.target.value)}
          autoComplete="new-password"
          minLength={PASSWORD_MIN}
          required
        />
      </label>
      <p className={controls.hint}>At least {PASSWORD_MIN} characters. A short sentence is easy to remember.</p>
      <label className={controls.field}>
        New password again
        <input
          type="password"
          className={controls.input}
          value={confirmation}
          onChange={(e) => setConfirmation(e.target.value)}
          autoComplete="new-password"
          required
        />
      </label>

      {error && <p className={controls.error}>{error}</p>}
      {isDone && (
        <p className={styles.success} role="status">
          Your password was changed. Every other device where you were logged in has been logged out.
        </p>
      )}

      <div className={controls.actions}>
        <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isSaving}>
          Change password
        </button>
      </div>
    </form>
  )
}

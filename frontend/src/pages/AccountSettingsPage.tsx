import { useState, type FormEvent } from 'react'
import { useNavigate } from 'react-router'
import Page from '../components/Page.tsx'
import LoginPrompt from '../components/LoginPrompt.tsx'
import controls from '../components/Controls.module.css'
import { PASSWORD_MIN, changePassword } from '../api/auth.ts'
import { useAuth } from '../context/authContext.ts'
import styles from './AccountPage.module.css'

// /account/settings: changing the password and deleting the account.
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
      <DeleteAccountForm />
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

function DeleteAccountForm() {
  const { deleteAccount } = useAuth()
  const navigate = useNavigate()
  const [password, setPassword] = useState('')
  const [isSure, setIsSure] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [isDeleting, setIsDeleting] = useState(false)

  async function handleSubmit(event: FormEvent) {
    event.preventDefault()
    setError(null)
    setIsDeleting(true)
    try {
      await deleteAccount(password)
      navigate('/', { state: { accountDeleted: true } })
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Something went wrong.')
      setIsDeleting(false)
    }
  }

  return (
    <form className={`${controls.form} ${styles.form}`} onSubmit={handleSubmit} aria-labelledby="delete-account-title">
      <h2 id="delete-account-title" className={styles.formTitle}>
        Delete account
      </h2>
      <p className={styles.warning}>
        This deletes your account and everything in it: your quote sets, your calendar, your reminders and the
        notifications on every device. It cannot be undone.
      </p>

      <label className={controls.field}>
        Your password
        <input
          type="password"
          className={controls.input}
          value={password}
          onChange={(e) => setPassword(e.target.value)}
          autoComplete="current-password"
          required
        />
      </label>
      <label className={styles.checkbox}>
        <input type="checkbox" checked={isSure} onChange={(e) => setIsSure(e.target.checked)} />
        I understand that this cannot be undone.
      </label>

      {error && <p className={controls.error}>{error}</p>}

      <div className={controls.actions}>
        <button
          type="submit"
          className={`${controls.button} ${controls.danger}`}
          disabled={!isSure || password === '' || isDeleting}
        >
          {isDeleting ? 'Deleting…' : 'Delete my account'}
        </button>
      </div>
    </form>
  )
}

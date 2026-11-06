import { useState, type FormEvent } from 'react'
import { Link, Navigate, useLocation, useNavigate } from 'react-router'
import Page from '../components/Page.tsx'
import controls from '../components/Controls.module.css'
import { PASSWORD_MIN } from '../api/auth.ts'
import { useAuth } from '../context/authContext.ts'
import styles from './AuthPage.module.css'

export default function RegisterPage() {
  const { user, register } = useAuth()
  const navigate = useNavigate()
  const location = useLocation()
  const from = (location.state as { from?: unknown } | null)?.from
  const returnPath = typeof from === 'string' && from.startsWith('/') ? from : '/quotes'

  const [username, setUsername] = useState('')
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [confirmation, setConfirmation] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [isSaving, setIsSaving] = useState(false)

  async function handleSubmit(event: FormEvent) {
    event.preventDefault()
    setError(null)

    // Checked here because only the browser sees both fields; the backend checks the rest.
    if (password !== confirmation) {
      setError('The two passwords are different.')
      return
    }

    setIsSaving(true)
    try {
      await register(email, username, password)
      navigate(returnPath, { replace: true })
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Something went wrong.')
      setIsSaving(false)
    }
  }

  if (user && !isSaving) {
    return <Navigate to={returnPath} replace />
  }

  return (
    <Page title="Create an account">
      <form className={`${controls.form} ${styles.form}`} onSubmit={handleSubmit}>
        <label className={controls.field}>
          Username
          <input
            className={controls.input}
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            autoComplete="username"
            minLength={2}
            maxLength={30}
            required
            autoFocus
          />
        </label>
        <label className={controls.field}>
          Email
          <input
            type="email"
            className={controls.input}
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            autoComplete="email"
            required
          />
        </label>
        <label className={controls.field}>
          Password
          <input
            type="password"
            className={controls.input}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            autoComplete="new-password"
            minLength={PASSWORD_MIN}
            required
          />
        </label>
        <p className={controls.hint}>At least {PASSWORD_MIN} characters. A short sentence is easy to remember.</p>
        <label className={controls.field}>
          Password again
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

        <div className={controls.actions}>
          <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isSaving}>
            Create account
          </button>
        </div>
      </form>

      <p className={styles.switch}>
        Already have an account?{' '}
        <Link to="/login" state={{ from: returnPath }}>
          Log in
        </Link>
      </p>
    </Page>
  )
}

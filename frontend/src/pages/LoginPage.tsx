import { useState, type FormEvent } from 'react'
import { Link, Navigate, useLocation, useNavigate } from 'react-router'
import Page from '../components/Page.tsx'
import controls from '../components/Controls.module.css'
import { useAuth } from '../context/authContext.ts'
import styles from './AuthPage.module.css'

// Where to go after logging in: back to the page that sent the visitor here, or Set quotes.
function useReturnPath(): string {
  const location = useLocation()
  const from = (location.state as { from?: unknown } | null)?.from
  return typeof from === 'string' && from.startsWith('/') ? from : '/quotes'
}

export default function LoginPage() {
  const { user, login } = useAuth()
  const navigate = useNavigate()
  const returnPath = useReturnPath()

  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [isSaving, setIsSaving] = useState(false)

  async function handleSubmit(event: FormEvent) {
    event.preventDefault()
    setError(null)
    setIsSaving(true)
    try {
      await login(email, password)
      navigate(returnPath, { replace: true })
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Something went wrong.')
      setIsSaving(false)
    }
  }

  // Someone already logged in has nothing to do here.
  if (user && !isSaving) {
    return <Navigate to={returnPath} replace />
  }

  return (
    <Page title="Log in">
      <form className={`${controls.form} ${styles.form}`} onSubmit={handleSubmit}>
        <label className={controls.field}>
          Email
          <input
            type="email"
            className={controls.input}
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            autoComplete="email"
            required
            autoFocus
          />
        </label>
        <label className={controls.field}>
          Password
          <input
            type="password"
            className={controls.input}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            autoComplete="current-password"
            required
          />
        </label>

        {error && <p className={controls.error}>{error}</p>}

        <div className={controls.actions}>
          <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isSaving}>
            Log in
          </button>
        </div>
      </form>

      <p className={styles.switch}>
        No account yet?{' '}
        <Link to="/register" state={{ from: returnPath }}>
          Create one
        </Link>
      </p>
    </Page>
  )
}

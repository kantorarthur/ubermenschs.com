import { useState } from 'react'
import { Link, useNavigate } from 'react-router'
import { useAuth } from '../context/authContext.ts'
import styles from './AuthButtons.module.css'

// Top right: "Log in" and "Register" for visitors, the username and "Log out" when logged in.
export default function AuthButtons() {
  const { user, isLoading, logout } = useAuth()
  const navigate = useNavigate()
  const [isLoggingOut, setIsLoggingOut] = useState(false)

  async function handleLogout() {
    setIsLoggingOut(true)
    try {
      await logout()
      navigate('/quotes')
    } finally {
      setIsLoggingOut(false)
    }
  }

  // Avoids showing "Log in" for a moment to someone who is already logged in.
  if (isLoading) {
    return <div className={styles.auth} />
  }

  if (user) {
    return (
      <div className={styles.auth}>
        <span className={styles.user}>{user.username}</span>
        <button type="button" className={styles.secondary} onClick={handleLogout} disabled={isLoggingOut}>
          Log out
        </button>
      </div>
    )
  }

  return (
    <nav aria-label="Account" className={styles.auth}>
      <Link to="/login" className={styles.secondary}>
        Log in
      </Link>
      <Link to="/register" className={styles.primary}>
        Register
      </Link>
    </nav>
  )
}

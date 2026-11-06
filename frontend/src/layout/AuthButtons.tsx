import { useEffect, useRef, useState } from 'react'
import { Link, useLocation, useNavigate } from 'react-router'
import { useAuth } from '../context/authContext.ts'
import styles from './AuthButtons.module.css'

// Top right: "Log in" and "Register" for visitors; for a logged-in user, a button with the
// username that opens the account menu (details, settings, log out).
export default function AuthButtons() {
  const { user, isLoading } = useAuth()

  // Avoids showing "Log in" for a moment to someone who is already logged in.
  if (isLoading) {
    return <div className={styles.auth} />
  }

  if (user) {
    return (
      <div className={styles.auth}>
        <AccountMenu username={user.username} />
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

function AccountMenu({ username }: { username: string }) {
  const { logout } = useAuth()
  const navigate = useNavigate()
  const location = useLocation()
  const [isOpen, setIsOpen] = useState(false)
  const [isLoggingOut, setIsLoggingOut] = useState(false)
  const container = useRef<HTMLDivElement>(null)
  const button = useRef<HTMLButtonElement>(null)

  // Opening another page closes the menu.
  const [openedAt, setOpenedAt] = useState(location.key)
  if (isOpen && openedAt !== location.key) {
    setIsOpen(false)
  }

  // A click anywhere else or Escape closes it.
  useEffect(() => {
    if (!isOpen) {
      return
    }
    function onPointerDown(event: PointerEvent) {
      if (!container.current?.contains(event.target as Node)) {
        setIsOpen(false)
      }
    }
    function onKeyDown(event: KeyboardEvent) {
      if (event.key === 'Escape') {
        setIsOpen(false)
        button.current?.focus()
      }
    }
    document.addEventListener('pointerdown', onPointerDown)
    document.addEventListener('keydown', onKeyDown)
    return () => {
      document.removeEventListener('pointerdown', onPointerDown)
      document.removeEventListener('keydown', onKeyDown)
    }
  }, [isOpen])

  function toggle() {
    setOpenedAt(location.key)
    setIsOpen((open) => !open)
  }

  async function handleLogout() {
    setIsLoggingOut(true)
    try {
      await logout()
      setIsOpen(false)
      navigate('/quotes')
    } finally {
      setIsLoggingOut(false)
    }
  }

  return (
    <div className={styles.menuContainer} ref={container}>
      <button
        type="button"
        ref={button}
        className={styles.accountButton}
        aria-haspopup="menu"
        aria-expanded={isOpen}
        aria-controls="account-menu"
        onClick={toggle}
      >
        <span className={styles.avatar} aria-hidden="true">
          {username.charAt(0).toUpperCase()}
        </span>
        <span className={styles.user}>{username}</span>
        <span className={styles.chevron} aria-hidden="true">
          ▾
        </span>
      </button>

      {isOpen && (
        <div id="account-menu" role="menu" aria-label="Account" className={styles.menu}>
          <Link role="menuitem" to="/account" className={styles.item} onClick={() => setIsOpen(false)}>
            Account details
          </Link>
          <Link role="menuitem" to="/account/settings" className={styles.item} onClick={() => setIsOpen(false)}>
            Account settings
          </Link>
          <div className={styles.separator} role="separator" />
          <button
            type="button"
            role="menuitem"
            className={styles.item}
            onClick={() => void handleLogout()}
            disabled={isLoggingOut}
          >
            Log out
          </button>
        </div>
      )}
    </div>
  )
}

import { Link, useLocation } from 'react-router'
import controls from './Controls.module.css'
import styles from './LoginPrompt.module.css'

// Shown to visitors where a feature needs an account.
// After logging in, the login page brings them back to this page.
export default function LoginPrompt({ children }: { children: string }) {
  const location = useLocation()

  return (
    <div className={styles.prompt}>
      <p>{children}</p>
      <div className={controls.actions}>
        <Link to="/login" state={{ from: location.pathname }} className={`${controls.button} ${controls.primary}`}>
          Log in
        </Link>
        <Link to="/register" state={{ from: location.pathname }} className={controls.button}>
          Create an account
        </Link>
      </div>
    </div>
  )
}

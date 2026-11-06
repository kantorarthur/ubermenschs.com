import { Link } from 'react-router'
import styles from './AuthButtons.module.css'

// Later: when someone is logged in, show their name and "Log out" instead.
export default function AuthButtons() {
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

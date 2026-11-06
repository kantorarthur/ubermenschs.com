import { Link } from 'react-router'
import styles from './Logo.module.css'

export default function Logo() {
  return (
    <Link to="/" className={styles.logo}>
      <span className={styles.mark} aria-hidden="true">
        Ü
      </span>
      <span className={styles.words}>
        <span className={styles.domain}>ubermenschs.com</span>
        <span className={styles.tagline}>
          One step closer to the Übermensch than you were yesterday.
        </span>
      </span>
    </Link>
  )
}

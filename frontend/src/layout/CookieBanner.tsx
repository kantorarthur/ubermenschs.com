import { Link, useLocation } from 'react-router'
import controls from '../components/Controls.module.css'
import { acceptCookie, dismissCookieNotice, useCookieConsent } from '../utils/cookieConsent.ts'
import styles from './CookieBanner.module.css'

const AUTH_PAGES = ['/login', '/register']

// The notice at the bottom of the screen until the cookie is accepted (see utils/cookieConsent.ts).
// On the login and registration pages it cannot be put away: logging in needs the cookie.
export default function CookieBanner() {
  const { accepted, dismissed } = useCookieConsent()
  const { pathname } = useLocation()
  const onAuthPage = AUTH_PAGES.includes(pathname)

  if (accepted || (dismissed && !onAuthPage)) {
    return null
  }

  return (
    <>
      <div className={styles.spacer} aria-hidden="true" />
      <section className={styles.banner} aria-labelledby="cookie-title">
        <div className={styles.text}>
          <h2 id="cookie-title" className={styles.title}>
            {onAuthPage ? 'Accept the cookie to log in' : 'One cookie. No tracking.'}
          </h2>
          <p>
            {onAuthPage
              ? 'Logging in needs the site’s single cookie, which keeps you logged in. No ads, no analytics.'
              : 'ubermenschs.com uses a single cookie, only to keep you logged in. No ads, no analytics, nothing sent to other companies.'}{' '}
            <Link to="/privacy" className={styles.link}>
              Privacy
            </Link>
          </p>
        </div>
        <div className={styles.buttons}>
          {!onAuthPage && (
            <button type="button" className={controls.button} onClick={dismissCookieNotice}>
              Not now
            </button>
          )}
          <button type="button" className={`${controls.button} ${controls.primary}`} onClick={acceptCookie}>
            Accept
          </button>
        </div>
      </section>
    </>
  )
}

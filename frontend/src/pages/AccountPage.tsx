import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import LoginPrompt from '../components/LoginPrompt.tsx'
import controls from '../components/Controls.module.css'
import { useAuth } from '../context/authContext.ts'
import styles from './AccountPage.module.css'

// "6 November 2026"
function formatDate(secondsSince1970: number): string {
  return new Date(secondsSince1970 * 1000).toLocaleDateString('en-GB', {
    day: 'numeric',
    month: 'long',
    year: 'numeric',
  })
}

// /account: who is logged in.
export default function AccountPage() {
  const { user, isLoading } = useAuth()

  if (isLoading) {
    return <Page title="Account details" />
  }
  if (!user) {
    return (
      <Page title="Account details">
        <LoginPrompt>Log in to see your account.</LoginPrompt>
      </Page>
    )
  }

  return (
    <Page title="Account details">
      <dl className={styles.details}>
        <div className={styles.row}>
          <dt>Username</dt>
          <dd>{user.username}</dd>
        </div>
        <div className={styles.row}>
          <dt>Email</dt>
          <dd>{user.email}</dd>
        </div>
        {user.createdAt > 0 && (
          <div className={styles.row}>
            <dt>Member since</dt>
            <dd>{formatDate(user.createdAt)}</dd>
          </div>
        )}
      </dl>

      <div className={controls.actions}>
        <Link to="/account/settings" className={controls.button}>
          Account settings
        </Link>
      </div>
    </Page>
  )
}

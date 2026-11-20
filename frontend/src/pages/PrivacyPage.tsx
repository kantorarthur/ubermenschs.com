import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import styles from './PrivacyPage.module.css'

const CONTACT = 'kantorarthur222@gmail.com'

// "/privacy" — what the site keeps about its users, where, for how long, and their rights (GDPR art. 13).
// When this text changes in a way users should know about, change CONSENT_VERSION (utils/cookieConsent.ts) too:
// the cookie notice then asks everyone again.
export default function PrivacyPage() {
  return (
    <Page title="Privacy" subtitle="Last updated: 20 November 2026">
      <div className={styles.text}>
        <section>
          <h2>Who runs this site</h2>
          <p>
            ubermenschs.com is run by Arthur Kantor, who decides what happens with the data described here. For any
            question or request about your data, write to <a href={`mailto:${CONTACT}`}>{CONTACT}</a>.
          </p>
        </section>

        <section>
          <h2>What is kept</h2>
          <p>Visitors without an account can read the whole site: nothing about them is saved.</p>
          <ul>
            <li>
              <strong>Your account:</strong> your email, your username and when the account was made. Your password is
              never stored, only an Argon2id hash of it, from which it cannot be read back.
            </li>
            <li>
              <strong>What you add:</strong> your quote sets, your calendar events, your daily reminders and your time
              zone (so they arrive at the right hour).
            </li>
            <li>
              <strong>Notifications:</strong> if you turn them on, the address your browser&apos;s push service gives
              for your device, and the keys used to encrypt the notifications for it.
            </li>
            <li>
              <strong>Protection against password guessing:</strong> the number of login and registration attempts
              from an IP address (kept for at most one hour) and of wrong passwords for an email address (15
              minutes).
            </li>
            <li>
              <strong>Server logs:</strong> the server writes down the requests it answers (time, the address asked
              for, the result). The hosting company keeps these logs for a limited time.
            </li>
          </ul>
        </section>

        <section>
          <h2>Cookies</h2>
          <p>
            There is <strong>one cookie</strong>, called <code>session</code>, and only after you log in or register.
            It holds a random code that keeps you logged in for 30 days or until you log out. The server keeps only a
            fingerprint of that code. JavaScript cannot read the cookie, and it is sent only over HTTPS.
          </p>
          <p>
            Your answer to the cookie notice is saved in your browser&apos;s local storage, so the notice does not
            come back on every page. There are no analytics, no ads and no trackers, and the fonts come from this site,
            not from Google.
          </p>
        </section>

        <section>
          <h2>Why</h2>
          <ul>
            <li>
              Your account data and what you add are used only to give you the site&apos;s features: your quotes, your
              calendar, your reminders (GDPR, article 6(1)(b): what you asked for when you made the account).
            </li>
            <li>
              Counting login attempts and the server logs protect the accounts and keep the site running (article
              6(1)(f): legitimate interest).
            </li>
          </ul>
          <p>Nothing is sold, shared for advertising or used to build a profile of you.</p>
        </section>

        <section>
          <h2>Where the data is</h2>
          <ul>
            <li>
              <strong>Render</strong> runs the server and the database, in Frankfurt, Germany (EU).
            </li>
            <li>
              <strong>Cloudflare</strong> delivers the website files and the domain&apos;s DNS. It sees your IP address
              when you open the site.
            </li>
            <li>
              <strong>Push services</strong> (Google, Mozilla, Apple or Microsoft, depending on your browser) deliver
              the notifications. Their content is encrypted end to end: only your browser can read it.
            </li>
          </ul>
          <p>
            Cloudflare and the push services are based in the United States and may process technical data such as IP
            addresses outside the EU, under their own data protection terms.
          </p>
        </section>

        <section>
          <h2>How long</h2>
          <p>
            Everything stays until you delete your account. Deleting it removes your account, sessions, quote sets,
            events, reminders and notification subscriptions at once. The server&apos;s disk has daily backups that
            are kept for 7 days, so deleted data is gone from them within 7 days.
          </p>
        </section>

        <section>
          <h2>Your rights</h2>
          <p>
            You can ask to see your data, correct it, receive a copy, or object to how it is used, by writing to{' '}
            <a href={`mailto:${CONTACT}`}>{CONTACT}</a>. You can delete your account yourself, at any time, in{' '}
            <Link to="/account/settings">Account settings</Link>.
          </p>
        </section>

        <section>
          <h2>Security</h2>
          <p>
            The site and its server are reached only over HTTPS. Passwords are stored as Argon2id hashes, logins as
            fingerprints, and notifications are encrypted for your browser. The rest of your data (such as event
            titles) is stored on the server without extra encryption.
          </p>
        </section>

        <section>
          <h2>Changes</h2>
          <p>If this page changes in a way that matters, the cookie notice will ask you again.</p>
        </section>
      </div>
    </Page>
  )
}

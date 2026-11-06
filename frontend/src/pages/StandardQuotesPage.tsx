import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import { standardQuotes } from '../data/standardQuotes.ts'
import styles from './StandardQuotesPage.module.css'

export default function StandardQuotesPage() {
  return (
    <Page title="Standard quotes" subtitle="The default preset, shown to everyone.">
      <Link to="/quotes" className={styles.back}>
        ← All presets
      </Link>
      <ul className={styles.list}>
        {standardQuotes.map((quote) => (
          <li key={quote.id} className={styles.quote}>
            <span className={styles.text}>“{quote.text}”</span>
            <span className={styles.author}>— {quote.author}</span>
          </li>
        ))}
      </ul>
    </Page>
  )
}

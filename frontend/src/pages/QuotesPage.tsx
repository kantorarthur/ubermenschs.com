import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import { standardQuotes } from '../data/standardQuotes.ts'
import styles from './QuotesPage.module.css'

export default function QuotesPage() {
  return (
    <Page title="Set quotes" subtitle="Choose the quotes that appear at the top of the site.">
      <ul className={styles.presets}>
        {/* "Standard quotes" is always first. The user's own presets will follow it. */}
        <li>
          <Link to="/quotes/standard" className={styles.preset}>
            <span className={styles.presetName}>Standard quotes</span>
            <span className={styles.presetCount}>{standardQuotes.length} quotes</span>
          </Link>
        </li>
      </ul>

      <button type="button" className={styles.addButton} disabled>
        + Add new quotes preset
      </button>
      <p className={styles.note}>Your own presets become available once accounts exist.</p>
    </Page>
  )
}

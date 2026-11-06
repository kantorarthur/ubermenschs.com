import { useState } from 'react'
import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import controls from '../components/Controls.module.css'
import { STANDARD_PRESET_ID } from '../api/presets.ts'
import { useQuotes } from '../context/quoteContext.ts'
import { standardQuotes } from '../data/standardQuotes.ts'
import styles from './StandardQuotesPage.module.css'

export default function StandardQuotesPage() {
  const { activePresetId, setActive } = useQuotes()
  const isActive = activePresetId === STANDARD_PRESET_ID
  const [error, setError] = useState<string | null>(null)

  async function chooseForHeader() {
    setError(null)
    try {
      await setActive(STANDARD_PRESET_ID)
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Something went wrong.')
    }
  }

  return (
    <Page title="Standard quotes" subtitle="The default preset, shown to everyone.">
      <Link to="/quotes" className={styles.back}>
        ← All presets
      </Link>

      <div className={controls.actions}>
        {isActive ? (
          <span className={controls.badge}>In the header</span>
        ) : (
          <button type="button" className={`${controls.button} ${controls.primary}`} onClick={chooseForHeader}>
            Use for the header
          </button>
        )}
      </div>

      {error && <p className={controls.error}>{error}</p>}

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

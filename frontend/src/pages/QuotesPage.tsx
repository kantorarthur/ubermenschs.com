import { useEffect, useState, type FormEvent } from 'react'
import { Link, useNavigate } from 'react-router'
import Page from '../components/Page.tsx'
import controls from '../components/Controls.module.css'
import { LIMITS, STANDARD_PRESET_ID, createPreset, getPresets } from '../api/presets.ts'
import { useQuotes } from '../context/quoteContext.ts'
import { standardQuotes } from '../data/standardQuotes.ts'
import type { QuotePreset } from '../types/models.ts'
import styles from './QuotesPage.module.css'

export default function QuotesPage() {
  const navigate = useNavigate()
  const { activePresetId } = useQuotes()

  const [presets, setPresets] = useState<QuotePreset[] | null>(null)
  const [loadError, setLoadError] = useState<string | null>(null)
  const [isAdding, setIsAdding] = useState(false)
  const [name, setName] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [isSaving, setIsSaving] = useState(false)

  // Load the user's presets when the page opens.
  useEffect(() => {
    const request = new AbortController()
    getPresets()
      .then((loaded) => {
        if (!request.signal.aborted) {
          setPresets(loaded)
        }
      })
      .catch((e: unknown) => {
        if (!request.signal.aborted) {
          setLoadError(e instanceof Error ? e.message : 'Could not load your presets.')
        }
      })
    return () => request.abort()
  }, [])

  async function handleCreate(event: FormEvent) {
    event.preventDefault()
    setError(null)
    setIsSaving(true)
    try {
      const preset = await createPreset(name)
      // Open the new preset right away, so quotes can be added to it.
      navigate(`/quotes/${preset.id}`)
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Something went wrong.')
      setIsSaving(false)
    }
  }

  function cancelAdding() {
    setIsAdding(false)
    setName('')
    setError(null)
  }

  return (
    <Page title="Set quotes" subtitle="Choose which quotes appear at the top of the site.">
      <ul className={styles.presets}>
        {/* "Standard quotes" is always first. */}
        <li>
          <Link to="/quotes/standard" className={styles.preset}>
            <span className={styles.presetName}>Standard quotes</span>
            <span className={styles.meta}>
              {activePresetId === STANDARD_PRESET_ID && (
                <span className={controls.badge}>In the header</span>
              )}
              <span className={styles.presetCount}>{standardQuotes.length} quotes</span>
            </span>
          </Link>
        </li>

        {presets?.map((preset) => (
          <li key={preset.id}>
            <Link to={`/quotes/${preset.id}`} className={styles.preset}>
              <span className={styles.presetName}>{preset.name}</span>
              <span className={styles.meta}>
                {activePresetId === preset.id && (
                  <span className={controls.badge}>In the header</span>
                )}
                <span className={styles.presetCount}>
                  {preset.quotes.length} {preset.quotes.length === 1 ? 'quote' : 'quotes'}
                </span>
              </span>
            </Link>
          </li>
        ))}
      </ul>

      {loadError && <p className={controls.error}>{loadError}</p>}

      {isAdding ? (
        <form className={controls.form} onSubmit={handleCreate}>
          <label className={controls.field}>
            Preset name
            <input
              className={controls.input}
              value={name}
              onChange={(e) => setName(e.target.value)}
              maxLength={LIMITS.presetName}
              placeholder="e.g. Stoics"
              autoFocus
            />
          </label>
          {error && <p className={controls.error}>{error}</p>}
          <div className={controls.actions}>
            <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isSaving}>
              Create preset
            </button>
            <button type="button" className={controls.button} onClick={cancelAdding}>
              Cancel
            </button>
          </div>
        </form>
      ) : (
        <button type="button" className={styles.addButton} onClick={() => setIsAdding(true)}>
          + Add new quotes preset
        </button>
      )}

      <p className={controls.hint}>
        Your presets are saved on the server. Until accounts exist, everyone using this copy of the site
        shares the same presets.
      </p>
    </Page>
  )
}

import { useEffect, useState, type FormEvent } from 'react'
import { Link, useNavigate, useParams } from 'react-router'
import Page from '../components/Page.tsx'
import LoginPrompt from '../components/LoginPrompt.tsx'
import controls from '../components/Controls.module.css'
import {
  LIMITS,
  addQuote,
  deletePreset,
  deleteQuote,
  getPreset,
  renamePreset,
} from '../api/presets.ts'
import { useAuth } from '../context/authContext.ts'
import { useQuotes } from '../context/quoteContext.ts'
import type { QuotePreset } from '../types/models.ts'
import styles from './PresetPage.module.css'

function errorMessage(e: unknown): string {
  return e instanceof Error ? e.message : 'Something went wrong.'
}

// One of the user's own presets: /quotes/:presetId
export default function PresetPage() {
  const { user, isLoading } = useAuth()

  if (isLoading) {
    return <Page title="Loading…" />
  }
  if (!user) {
    return (
      <Page title="Quotes preset">
        <LoginPrompt>Log in to see and edit your quote presets.</LoginPrompt>
      </Page>
    )
  }
  // key: switching users or presets starts with fresh data.
  return <PresetEditor key={user.id} />
}

function PresetEditor() {
  const { presetId = '' } = useParams()
  const navigate = useNavigate()
  const { activePresetId, setActive, refresh } = useQuotes()

  // undefined = still loading, null = this preset does not exist
  const [preset, setPreset] = useState<QuotePreset | null | undefined>(undefined)
  const [loadError, setLoadError] = useState<string | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [isBusy, setIsBusy] = useState(false)

  const [text, setText] = useState('')
  const [author, setAuthor] = useState('')

  const [isRenaming, setIsRenaming] = useState(false)
  const [newName, setNewName] = useState('')

  const isActive = activePresetId === presetId

  // Load the preset whenever the URL points to a different one.
  useEffect(() => {
    const request = new AbortController()
    getPreset(presetId)
      .then((loaded) => {
        if (!request.signal.aborted) {
          setPreset(loaded ?? null)
        }
      })
      .catch((e: unknown) => {
        if (!request.signal.aborted) {
          setLoadError(errorMessage(e))
        }
      })
    return () => request.abort()
  }, [presetId])

  // Runs a change, then reloads the preset (and the header, if this preset feeds it).
  async function run(change: () => Promise<unknown>): Promise<boolean> {
    setError(null)
    setIsBusy(true)
    try {
      await change()
      setPreset((await getPreset(presetId)) ?? null)
      if (isActive) {
        await refresh()
      }
      return true
    } catch (e) {
      setError(errorMessage(e))
      return false
    } finally {
      setIsBusy(false)
    }
  }

  async function handleAddQuote(event: FormEvent) {
    event.preventDefault()
    const ok = await run(() => addQuote(presetId, { text, author }))
    if (ok) {
      setText('')
      setAuthor('')
    }
  }

  async function handleRename(event: FormEvent) {
    event.preventDefault()
    const ok = await run(() => renamePreset(presetId, newName))
    if (ok) {
      setIsRenaming(false)
    }
  }

  async function handleDeletePreset() {
    if (!preset || !window.confirm(`Delete the preset “${preset.name}” and all its quotes?`)) {
      return
    }
    setIsBusy(true)
    try {
      await deletePreset(presetId)
      await refresh()
      navigate('/quotes')
    } catch (e) {
      setError(errorMessage(e))
      setIsBusy(false)
    }
  }

  async function handleUseForHeader() {
    setError(null)
    try {
      await setActive(presetId)
    } catch (e) {
      setError(errorMessage(e))
    }
  }

  if (preset === undefined && loadError) {
    return (
      <Page title="Could not load this preset">
        <p className={controls.error}>{loadError}</p>
        <Link to="/quotes" className={styles.back}>
          ← All presets
        </Link>
      </Page>
    )
  }

  if (preset === undefined) {
    return <Page title="Loading…" />
  }

  if (preset === null) {
    return (
      <Page title="Preset not found" subtitle="It may have been deleted.">
        <Link to="/quotes" className={styles.back}>
          ← All presets
        </Link>
      </Page>
    )
  }

  return (
    <Page title={preset.name}>
      <Link to="/quotes" className={styles.back}>
        ← All presets
      </Link>

      <div className={controls.actions}>
        {isActive ? (
          <span className={controls.badge}>In the header</span>
        ) : (
          <button type="button" className={`${controls.button} ${controls.primary}`} onClick={handleUseForHeader}>
            Use for the header
          </button>
        )}
        <button
          type="button"
          className={controls.button}
          onClick={() => {
            setNewName(preset.name)
            setIsRenaming(true)
          }}
        >
          Rename
        </button>
        <button
          type="button"
          className={`${controls.button} ${controls.danger}`}
          onClick={handleDeletePreset}
          disabled={isBusy}
        >
          Delete preset
        </button>
      </div>

      {isActive && preset.quotes.length === 0 && (
        <p className={controls.hint}>This preset is empty, so the header shows Standard quotes until you add some.</p>
      )}

      {isRenaming && (
        <form className={controls.form} onSubmit={handleRename}>
          <label className={controls.field}>
            New name
            <input
              className={controls.input}
              value={newName}
              onChange={(e) => setNewName(e.target.value)}
              maxLength={LIMITS.presetName}
              autoFocus
            />
          </label>
          <div className={controls.actions}>
            <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isBusy}>
              Save name
            </button>
            <button type="button" className={controls.button} onClick={() => setIsRenaming(false)}>
              Cancel
            </button>
          </div>
        </form>
      )}

      <form className={controls.form} onSubmit={handleAddQuote}>
        <label className={controls.field}>
          Quote
          <textarea
            className={controls.input}
            value={text}
            onChange={(e) => setText(e.target.value)}
            maxLength={LIMITS.quoteText}
            placeholder="Write the quote, without quotation marks"
          />
        </label>
        <label className={controls.field}>
          Author
          <input
            className={controls.input}
            value={author}
            onChange={(e) => setAuthor(e.target.value)}
            maxLength={LIMITS.quoteAuthor}
            placeholder="e.g. Marcus Aurelius"
          />
        </label>
        <div className={controls.actions}>
          <button type="submit" className={`${controls.button} ${controls.primary}`} disabled={isBusy}>
            Add quote
          </button>
        </div>
      </form>

      {error && <p className={controls.error}>{error}</p>}

      {preset.quotes.length === 0 ? (
        <p className={controls.hint}>No quotes yet. Add the first one above.</p>
      ) : (
        <ul className={styles.list}>
          {preset.quotes.map((quote) => (
            <li key={quote.id} className={styles.quote}>
              <div className={styles.quoteBody}>
                <span className={styles.text}>“{quote.text}”</span>
                <span className={styles.author}>— {quote.author}</span>
              </div>
              <button
                type="button"
                className={`${controls.button} ${controls.small}`}
                onClick={() => void run(() => deleteQuote(presetId, quote.id))}
                disabled={isBusy}
                aria-label={`Delete the quote by ${quote.author}`}
              >
                Delete
              </button>
            </li>
          ))}
        </ul>
      )}
    </Page>
  )
}

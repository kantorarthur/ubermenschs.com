import { useCallback, useEffect, useMemo, useState, type ReactNode } from 'react'
import { STANDARD_PRESET_ID, getActivePresetId, getPreset, setActivePreset } from '../api/presets.ts'
import { standardQuotes } from '../data/standardQuotes.ts'
import type { Quote } from '../types/models.ts'
import { QuoteContext, type QuoteContextValue } from './quoteContext.ts'

// Asks the API which preset is active and returns the quotes the header should use.
async function loadHeaderQuotes(): Promise<{ presetId: string; quotes: Quote[] }> {
  const presetId = await getActivePresetId()

  if (presetId !== STANDARD_PRESET_ID) {
    const preset = await getPreset(presetId)
    // An empty preset would leave the header blank, so Standard fills in until it has quotes.
    if (preset && preset.quotes.length > 0) {
      return { presetId, quotes: preset.quotes }
    }
  }

  return { presetId, quotes: standardQuotes }
}

// Keeps track of the active preset for the whole app.
// The header reads it; the Set quotes pages change it.
export function QuoteProvider({ children }: { children: ReactNode }) {
  const [activePresetId, setActivePresetId] = useState(STANDARD_PRESET_ID)
  const [headerQuotes, setHeaderQuotes] = useState<Quote[]>(standardQuotes)
  const [version, setVersion] = useState(0)

  const apply = useCallback((loaded: { presetId: string; quotes: Quote[] }) => {
    setActivePresetId(loaded.presetId)
    setHeaderQuotes(loaded.quotes)
    setVersion((v) => v + 1)
  }, [])

  const refresh = useCallback(async () => {
    apply(await loadHeaderQuotes())
  }, [apply])

  const setActive = useCallback(
    async (presetId: string) => {
      await setActivePreset(presetId)
      await refresh()
    },
    [refresh],
  )

  // Load the saved choice once, when the app starts.
  useEffect(() => {
    let cancelled = false
    void loadHeaderQuotes().then((loaded) => {
      if (!cancelled) {
        apply(loaded)
      }
    })
    return () => {
      cancelled = true
    }
  }, [apply])

  const value = useMemo<QuoteContextValue>(
    () => ({ activePresetId, headerQuotes, version, setActive, refresh }),
    [activePresetId, headerQuotes, version, setActive, refresh],
  )

  return <QuoteContext.Provider value={value}>{children}</QuoteContext.Provider>
}

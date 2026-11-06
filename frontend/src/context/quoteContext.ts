import { createContext, useContext } from 'react'
import type { Quote } from '../types/models.ts'

export interface QuoteContextValue {
  // Which preset feeds the header: 'standard' or the id of a user preset.
  activePresetId: string
  // The quotes the header picks from (the active preset's, or Standard as a fallback).
  headerQuotes: Quote[]
  // Goes up every time headerQuotes is reloaded, so the header can pick again.
  version: number
  // Makes a preset the active one.
  setActive: (presetId: string) => Promise<void>
  // Reloads the active preset after its quotes changed.
  refresh: () => Promise<void>
}

export const QuoteContext = createContext<QuoteContextValue | null>(null)

// Lets any component read and change the active preset.
export function useQuotes(): QuoteContextValue {
  const value = useContext(QuoteContext)
  if (!value) {
    throw new Error('useQuotes must be used inside <QuoteProvider>.')
  }
  return value
}

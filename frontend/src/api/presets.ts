import type { Quote, QuotePreset } from '../types/models.ts'
import { ApiError, apiFetch } from './client.ts'

// The user's own quote presets, stored by the C++ backend in Redis.
// Every function is one request to the API described in "Contractul API" in the plan.

export const STANDARD_PRESET_ID = 'standard'

// The backend checks the same limits; here they only set the inputs' maxLength.
export const LIMITS = {
  presetName: 60,
  quoteText: 400,
  quoteAuthor: 100,
}

// GET /api/presets
export function getPresets(): Promise<QuotePreset[]> {
  return apiFetch<QuotePreset[]>('/api/presets')
}

// GET /api/presets/:id  — undefined if it does not exist
export async function getPreset(presetId: string): Promise<QuotePreset | undefined> {
  try {
    return await apiFetch<QuotePreset>(`/api/presets/${encodeURIComponent(presetId)}`)
  } catch (error) {
    if (error instanceof ApiError && error.status === 404) {
      return undefined
    }
    throw error
  }
}

// POST /api/presets
export function createPreset(name: string): Promise<QuotePreset> {
  return apiFetch<QuotePreset>('/api/presets', { method: 'POST', body: { name } })
}

// PATCH /api/presets/:id
export function renamePreset(presetId: string, name: string): Promise<QuotePreset> {
  return apiFetch<QuotePreset>(`/api/presets/${encodeURIComponent(presetId)}`, {
    method: 'PATCH',
    body: { name },
  })
}

// DELETE /api/presets/:id
export function deletePreset(presetId: string): Promise<void> {
  return apiFetch<void>(`/api/presets/${encodeURIComponent(presetId)}`, { method: 'DELETE' })
}

// POST /api/presets/:id/quotes
export function addQuote(presetId: string, input: { text: string; author: string }): Promise<Quote> {
  return apiFetch<Quote>(`/api/presets/${encodeURIComponent(presetId)}/quotes`, {
    method: 'POST',
    body: input,
  })
}

// DELETE /api/presets/:id/quotes/:quoteId
export function deleteQuote(presetId: string, quoteId: string): Promise<void> {
  return apiFetch<void>(
    `/api/presets/${encodeURIComponent(presetId)}/quotes/${encodeURIComponent(quoteId)}`,
    { method: 'DELETE' },
  )
}

// GET /api/settings: which preset feeds the header.
export async function getActivePresetId(): Promise<string> {
  const settings = await apiFetch<{ activePreset: string }>('/api/settings')
  return settings.activePreset
}

// PUT /api/settings/active-preset
export function setActivePreset(presetId: string): Promise<void> {
  return apiFetch<void>('/api/settings/active-preset', { method: 'PUT', body: { presetId } })
}

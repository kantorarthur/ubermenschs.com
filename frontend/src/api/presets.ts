import type { Quote, QuotePreset } from '../types/models.ts'

// The user's own quote presets.
//
// TEMPORARY: the data lives in memory, so it disappears on refresh.
// Every function already has the shape of the real API call (see "Contractul API" in the plan);
// when the C++ backend exists, only the inside of these functions changes to fetch('/api/...').

export const STANDARD_PRESET_ID = 'standard'

export const LIMITS = {
  presetName: 60,
  quoteText: 400,
  quoteAuthor: 100,
}

let presets: QuotePreset[] = []
let activePresetId = STANDARD_PRESET_ID
let nextId = 1

function newId(prefix: string): string {
  const id = `${prefix}-${nextId}`
  nextId += 1
  return id
}

// A real request takes time; this makes the pages behave the same way already.
function networkDelay(): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, 120))
}

// Callers get copies, so they can never change the stored data by accident.
function copy<T>(value: T): T {
  return structuredClone(value)
}

function findPreset(presetId: string): QuotePreset {
  const preset = presets.find((p) => p.id === presetId)
  if (!preset) {
    throw new Error('This preset does not exist.')
  }
  return preset
}

function cleanName(name: string): string {
  const trimmed = name.trim()
  if (trimmed.length === 0) {
    throw new Error('The preset needs a name.')
  }
  if (trimmed.length > LIMITS.presetName) {
    throw new Error(`The name can have at most ${LIMITS.presetName} characters.`)
  }
  return trimmed
}

// GET /api/presets
export async function getPresets(): Promise<QuotePreset[]> {
  await networkDelay()
  return copy(presets)
}

// One preset, or undefined if it does not exist.
export async function getPreset(presetId: string): Promise<QuotePreset | undefined> {
  await networkDelay()
  const preset = presets.find((p) => p.id === presetId)
  return preset ? copy(preset) : undefined
}

// POST /api/presets
export async function createPreset(name: string): Promise<QuotePreset> {
  await networkDelay()
  const cleanedName = cleanName(name)
  const preset: QuotePreset = { id: newId('preset'), name: cleanedName, quotes: [] }
  presets.push(preset)
  return copy(preset)
}

// PATCH /api/presets/:id
export async function renamePreset(presetId: string, name: string): Promise<QuotePreset> {
  await networkDelay()
  const preset = findPreset(presetId)
  preset.name = cleanName(name)
  return copy(preset)
}

// DELETE /api/presets/:id
export async function deletePreset(presetId: string): Promise<void> {
  await networkDelay()
  findPreset(presetId)
  presets = presets.filter((p) => p.id !== presetId)
  if (activePresetId === presetId) {
    activePresetId = STANDARD_PRESET_ID
  }
}

// POST /api/presets/:id/quotes
export async function addQuote(
  presetId: string,
  input: { text: string; author: string },
): Promise<Quote> {
  await networkDelay()
  const preset = findPreset(presetId)

  const text = input.text.trim()
  const author = input.author.trim()
  if (text.length === 0) {
    throw new Error('The quote cannot be empty.')
  }
  if (text.length > LIMITS.quoteText) {
    throw new Error(`The quote can have at most ${LIMITS.quoteText} characters.`)
  }
  if (author.length === 0) {
    throw new Error('Add the author of the quote.')
  }
  if (author.length > LIMITS.quoteAuthor) {
    throw new Error(`The author can have at most ${LIMITS.quoteAuthor} characters.`)
  }

  const quote: Quote = { id: newId('quote'), text, author }
  preset.quotes.push(quote)
  return copy(quote)
}

// DELETE /api/presets/:id/quotes/:quoteId
export async function deleteQuote(presetId: string, quoteId: string): Promise<void> {
  await networkDelay()
  const preset = findPreset(presetId)
  preset.quotes = preset.quotes.filter((q) => q.id !== quoteId)
}

// Part of GET /api/settings: which preset feeds the header.
export async function getActivePresetId(): Promise<string> {
  await networkDelay()
  return activePresetId
}

// PUT /api/settings/active-preset
export async function setActivePreset(presetId: string): Promise<void> {
  await networkDelay()
  if (presetId !== STANDARD_PRESET_ID) {
    findPreset(presetId)
  }
  activePresetId = presetId
}

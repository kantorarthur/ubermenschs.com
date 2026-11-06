import { useState } from 'react'

// Picks a random item, but never the same one as `previous`
// (unless there is only one item to choose from).
export function pickRandom<T>(items: readonly T[], previous?: T): T | undefined {
  if (items.length === 0) {
    return undefined
  }
  if (items.length === 1) {
    return items[0]
  }

  const choices = items.filter((item) => item !== previous)
  const index = Math.floor(Math.random() * choices.length)
  return choices[index]
}

// Returns a random item from `items` and picks a new one every time `changeKey` changes.
// The new pick is never the same as the one shown just before.
//
// It uses React's "adjust state while rendering" pattern: when the key differs from the
// one we stored, we pick again in the same render, so the page never shows an old pick.
export function useRandomOnChange<T>(items: readonly T[], changeKey: unknown): T | undefined {
  const [state, setState] = useState(() => ({
    key: changeKey,
    value: pickRandom(items),
  }))

  if (state.key !== changeKey) {
    const next = { key: changeKey, value: pickRandom(items, state.value) }
    setState(next)
    return next.value
  }

  return state.value
}

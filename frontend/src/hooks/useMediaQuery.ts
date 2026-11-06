import { useSyncExternalStore } from 'react'

// True while the CSS media query matches, e.g. useMediaQuery('(orientation: portrait)').
// Re-renders the component when the answer changes (phone rotated, window resized).
export function useMediaQuery(query: string): boolean {
  return useSyncExternalStore(
    (onChange) => {
      const list = window.matchMedia(query)
      list.addEventListener('change', onChange)
      return () => list.removeEventListener('change', onChange)
    },
    () => window.matchMedia(query).matches,
  )
}

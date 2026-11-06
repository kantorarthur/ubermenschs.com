import { renderHook } from '@testing-library/react'
import { describe, expect, it } from 'vitest'
import { pickRandom, useRandomOnChange } from './useRandomOnChange.ts'

describe('pickRandom', () => {
  it('returns undefined for an empty list', () => {
    expect(pickRandom([])).toBeUndefined()
  })

  it('returns the only item, even if it was the previous one', () => {
    expect(pickRandom(['a'], 'a')).toBe('a')
  })

  it('never returns the previous item when there is a choice', () => {
    for (let i = 0; i < 500; i += 1) {
      expect(pickRandom(['a', 'b'], 'a')).toBe('b')
      expect(pickRandom(['a', 'b', 'c'], 'b')).not.toBe('b')
    }
  })

  it('reaches every other item', () => {
    const seen = new Set<string>()
    for (let i = 0; i < 500; i += 1) {
      seen.add(pickRandom(['a', 'b', 'c', 'd'], 'a')!)
    }
    expect([...seen].sort()).toEqual(['b', 'c', 'd'])
  })
})

describe('useRandomOnChange', () => {
  const items = ['a', 'b', 'c']

  it('keeps the same pick while the key stays the same', () => {
    const { result, rerender } = renderHook(({ key }) => useRandomOnChange(items, key), {
      initialProps: { key: 'page-1' },
    })
    const first = result.current
    for (let i = 0; i < 20; i += 1) {
      rerender({ key: 'page-1' })
      expect(result.current).toBe(first)
    }
  })

  it('picks a different item every time the key changes', () => {
    const { result, rerender } = renderHook(({ key }) => useRandomOnChange(items, key), {
      initialProps: { key: 0 },
    })
    for (let i = 1; i <= 50; i += 1) {
      const before = result.current
      rerender({ key: i })
      expect(items).toContain(result.current)
      expect(result.current).not.toBe(before)
    }
  })
})

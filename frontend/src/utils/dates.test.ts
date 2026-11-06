import { describe, expect, it } from 'vitest'
import { dateKeyOf, formatDayLong, formatMonth, monthGrid, timeOf, toDateKey } from './dates.ts'

describe('toDateKey', () => {
  it('formats a local date as YYYY-MM-DD with leading zeros', () => {
    expect(toDateKey(new Date(2026, 0, 5))).toBe('2026-01-05')
    expect(toDateKey(new Date(2026, 10, 30))).toBe('2026-11-30')
  })

  it('keeps the local day late in the evening (no shift to UTC)', () => {
    expect(toDateKey(new Date(2026, 10, 6, 23, 59))).toBe('2026-11-06')
  })
})

describe('dateKeyOf and timeOf', () => {
  it('split a local date-time', () => {
    expect(dateKeyOf('2026-11-06T14:30')).toBe('2026-11-06')
    expect(timeOf('2026-11-06T14:30')).toBe('14:30')
  })
})

describe('formatMonth and formatDayLong', () => {
  it('write months and days in words', () => {
    expect(formatMonth(2026, 10)).toBe('November 2026')
    expect(formatDayLong('2026-11-06')).toBe('Friday, 6 November 2026')
  })
})

describe('monthGrid', () => {
  it('starts on a Monday and ends on a Sunday', () => {
    const days = monthGrid(2026, 10) // November 2026
    expect(days[0].getDay()).toBe(1)
    expect(days[days.length - 1].getDay()).toBe(0)
    expect(days.length % 7).toBe(0)
  })

  it('fills the first week with days of the previous month', () => {
    // 1 November 2026 is a Sunday, so the grid starts on Monday 26 October.
    const days = monthGrid(2026, 10)
    expect(toDateKey(days[0])).toBe('2026-10-26')
    expect(toDateKey(days[6])).toBe('2026-11-01')
    expect(days.length).toBe(42)
  })

  it('uses exactly 4 weeks for a February that starts on a Monday', () => {
    const days = monthGrid(2027, 1) // 1 February 2027 is a Monday
    expect(days.length).toBe(28)
    expect(toDateKey(days[0])).toBe('2027-02-01')
    expect(toDateKey(days[27])).toBe('2027-02-28')
  })

  it('contains every day of the month once', () => {
    const keys = monthGrid(2028, 1).map(toDateKey) // February 2028 has 29 days
    for (let day = 1; day <= 29; day += 1) {
      const key = `2028-02-${String(day).padStart(2, '0')}`
      expect(keys.filter((k) => k === key)).toHaveLength(1)
    }
  })
})

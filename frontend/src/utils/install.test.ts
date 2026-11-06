import { describe, expect, it } from 'vitest'
import { installHelp } from './install.ts'

const UA = {
  iphone:
    'Mozilla/5.0 (iPhone; CPU iPhone OS 18_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.1 Mobile/15E148 Safari/604.1',
  ipad: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.1 Safari/605.1.15',
  macSafari:
    'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.1 Safari/605.1.15',
  macChrome:
    'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36',
  firefox: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:144.0) Gecko/20100101 Firefox/144.0',
  edge: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36 Edg/141.0.0.0',
}

describe('installHelp', () => {
  it('knows the browsers without an install window', () => {
    expect(installHelp(UA.iphone, 5)).toBe('ios')
    expect(installHelp(UA.ipad, 5)).toBe('ios') // iPadOS says it is a Mac, but has a touch screen
    expect(installHelp(UA.macSafari, 0)).toBe('mac-safari')
    expect(installHelp(UA.firefox, 0)).toBe('firefox')
  })

  it('uses the general steps for the others', () => {
    expect(installHelp(UA.macChrome, 0)).toBe('other')
    expect(installHelp(UA.edge, 0)).toBe('other')
  })
})

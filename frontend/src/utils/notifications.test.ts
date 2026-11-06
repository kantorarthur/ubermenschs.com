import { afterEach, describe, expect, it, vi } from 'vitest'
import { base64UrlToBytes, forgetSubscriptionOnServer, notificationState, pushSupported } from './notifications.ts'

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('base64UrlToBytes', () => {
  it('turns base64url text into bytes, with or without padding', () => {
    expect(Array.from(base64UrlToBytes('Zm9vYg'))).toEqual([102, 111, 111, 98])
    expect(Array.from(base64UrlToBytes('-_8'))).toEqual([0xfb, 0xff])
  })

  it('decodes a 65-byte VAPID public key', () => {
    const key = 'BP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A8'
    const bytes = base64UrlToBytes(key)
    expect(bytes.length).toBe(65)
    expect(bytes[0]).toBe(4) // an uncompressed P-256 point starts with 0x04
  })
})

describe('in a browser without push (jsdom, Safari outside the home screen)', () => {
  it('reports "unsupported"', async () => {
    expect(pushSupported()).toBe(false)
    await expect(notificationState()).resolves.toBe('unsupported')
  })

  it('logging out does not call the backend', async () => {
    const fetchMock = vi.fn()
    vi.stubGlobal('fetch', fetchMock)
    await forgetSubscriptionOnServer()
    expect(fetchMock).not.toHaveBeenCalled()
  })
})

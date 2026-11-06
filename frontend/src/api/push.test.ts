import { afterEach, describe, expect, it, vi } from 'vitest'
import { getPushPublicKey, removeSubscription, saveSubscription, sendTestNotification } from './push.ts'

function answer(status: number, body: unknown) {
  const fetchMock = vi.fn((_url: string, _init?: RequestInit) =>
    Promise.resolve(new Response(status === 204 ? null : JSON.stringify(body), { status })),
  )
  vi.stubGlobal('fetch', fetchMock)
  return fetchMock
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('push API', () => {
  it('reads the public key', async () => {
    answer(200, { publicKey: 'BAz1' })
    await expect(getPushPublicKey()).resolves.toBe('BAz1')
  })

  it('sends the subscription exactly as the browser gives it', async () => {
    const fetchMock = answer(201, { id: 'abc' })
    const subscription = {
      endpoint: 'https://fcm.googleapis.com/fcm/send/1',
      expirationTime: null,
      keys: { p256dh: 'BCV', auth: 'BTB' },
    }
    await saveSubscription(subscription)
    expect(fetchMock.mock.calls[0][0]).toBe('/api/push/subscriptions')
    expect(JSON.parse(String(fetchMock.mock.calls[0][1]?.body))).toEqual(subscription)
  })

  it('unsubscribes by endpoint', async () => {
    const fetchMock = answer(204, null)
    await removeSubscription('https://fcm.googleapis.com/fcm/send/1')
    expect(fetchMock.mock.calls[0][0]).toBe('/api/push/unsubscribe')
    expect(JSON.parse(String(fetchMock.mock.calls[0][1]?.body))).toEqual({
      endpoint: 'https://fcm.googleapis.com/fcm/send/1',
    })
  })

  it('passes on the rate limit message of the test button', async () => {
    answer(429, { error: 'Too many attempts. Try again in 10 minutes.' })
    await expect(sendTestNotification()).rejects.toThrow('Too many attempts. Try again in 10 minutes.')
  })
})

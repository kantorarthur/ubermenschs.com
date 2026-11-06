import { afterEach, describe, expect, it, vi } from 'vitest'
import { browserTimeZone, createReminder, updateReminder } from './reminders.ts'

function answer(status: number, body: unknown) {
  const fetchMock = vi.fn((_url: string, _init?: RequestInit) =>
    Promise.resolve(new Response(JSON.stringify(body), { status })),
  )
  vi.stubGlobal('fetch', fetchMock)
  return fetchMock
}

// The JSON body of the first request.
function sentBody(fetchMock: ReturnType<typeof answer>): unknown {
  const init = fetchMock.mock.calls[0][1]
  return JSON.parse(String(init?.body))
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('reminders API', () => {
  it("creates a reminder in the browser's time zone", async () => {
    const fetchMock = answer(201, {})
    await createReminder({ time: '07:00', message: 'Read 20 pages' })
    expect(sentBody(fetchMock)).toEqual({ time: '07:00', message: 'Read 20 pages', timezone: browserTimeZone() })
  })

  it('sends the time zone along with a new time', async () => {
    const fetchMock = answer(200, {})
    await updateReminder('3', { time: '06:30' })
    expect(fetchMock.mock.calls[0][0]).toBe('/api/reminders/3')
    expect(sentBody(fetchMock)).toEqual({ time: '06:30', timezone: browserTimeZone() })
  })

  it('switching a reminder off sends only that', async () => {
    const fetchMock = answer(200, {})
    await updateReminder('3', { enabled: false })
    expect(sentBody(fetchMock)).toEqual({ enabled: false })
  })

  it("passes on the backend's message", async () => {
    answer(400, { error: 'You can have at most 20 reminders.' })
    await expect(createReminder({ time: '07:00', message: 'One more' })).rejects.toThrow(
      'You can have at most 20 reminders.',
    )
  })
})

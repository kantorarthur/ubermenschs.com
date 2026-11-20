import { afterEach, describe, expect, it, vi } from 'vitest'
import { createEvent, deleteEvent, getEvents, updateEvent } from './events.ts'
import { browserTimeZone } from './reminders.ts'

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

const seminar = {
  title: 'Seminar',
  start: '2026-11-06T10:00',
  end: '2026-11-06T12:00',
  allDay: false,
  description: '',
  notify: [10, 1440],
}

describe('events API', () => {
  it('asks for the days on screen', async () => {
    const fetchMock = answer(200, [])
    await getEvents('2026-10-26', '2026-12-06')
    expect(fetchMock.mock.calls[0][0]).toBe('/api/events?from=2026-10-26&to=2026-12-06')
  })

  it("sends a new event as JSON, with the browser's time zone for its notifications", async () => {
    const fetchMock = answer(201, { id: '1', ...seminar, timezone: 'Europe/Bucharest' })
    await expect(createEvent(seminar)).resolves.toEqual({ id: '1', ...seminar, timezone: 'Europe/Bucharest' })
    expect(fetchMock.mock.calls[0][0]).toBe('/api/events')
    expect(fetchMock.mock.calls[0][1]?.method).toBe('POST')
    expect(JSON.parse(String(fetchMock.mock.calls[0][1]?.body))).toEqual({ ...seminar, timezone: browserTimeZone() })
  })

  it('updates and deletes by id', async () => {
    let fetchMock = answer(200, { id: '7', ...seminar })
    await updateEvent('7', seminar)
    expect(fetchMock).toHaveBeenCalledWith('/api/events/7', expect.objectContaining({ method: 'PATCH' }))

    fetchMock = answer(204, null)
    await expect(deleteEvent('7')).resolves.toBeUndefined()
    expect(fetchMock).toHaveBeenCalledWith('/api/events/7', expect.objectContaining({ method: 'DELETE' }))
  })

  it("passes on the backend's message", async () => {
    answer(400, { error: 'The event cannot end before it starts.' })
    await expect(createEvent(seminar)).rejects.toThrow('The event cannot end before it starts.')
  })
})

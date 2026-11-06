import { afterEach, describe, expect, it, vi } from 'vitest'
import { ApiError, apiFetch } from './client.ts'

// Replaces the browser's fetch with one that gives a prepared answer.
function mockFetch(response: Response | Error) {
  const fetchMock = vi.fn(() => (response instanceof Error ? Promise.reject(response) : Promise.resolve(response)))
  vi.stubGlobal('fetch', fetchMock)
  return fetchMock
}

function json(status: number, body: unknown): Response {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('apiFetch', () => {
  it('returns the JSON answer', async () => {
    mockFetch(json(200, { id: '1', name: 'Stoics' }))
    await expect(apiFetch('/api/presets/1')).resolves.toEqual({ id: '1', name: 'Stoics' })
  })

  it('returns undefined for 204 No Content', async () => {
    mockFetch(new Response(null, { status: 204 }))
    await expect(apiFetch('/api/presets/1', { method: 'DELETE' })).resolves.toBeUndefined()
  })

  it('sends JSON, the method and the login cookie', async () => {
    const fetchMock = mockFetch(json(201, {}))
    await apiFetch('/api/presets', { method: 'POST', body: { name: 'Stoics' } })

    expect(fetchMock).toHaveBeenCalledWith('/api/presets', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: '{"name":"Stoics"}',
      credentials: 'include',
    })
  })

  it("turns the backend's error into an ApiError with its message and status", async () => {
    mockFetch(json(409, { error: 'An account with this email already exists.' }))
    const error = await apiFetch('/api/auth/register').catch((e: unknown) => e)

    expect(error).toBeInstanceOf(ApiError)
    expect((error as ApiError).status).toBe(409)
    expect((error as ApiError).message).toBe('An account with this email already exists.')
  })

  it('keeps the rate limit message (429)', async () => {
    mockFetch(json(429, { error: 'Too many attempts. Try again in 15 minutes.' }))
    await expect(apiFetch('/api/auth/login')).rejects.toThrow('Too many attempts. Try again in 15 minutes.')
  })

  it('explains a 502 from the proxy as an unreachable server', async () => {
    mockFetch(new Response('Bad Gateway', { status: 502 }))
    await expect(apiFetch('/api/presets')).rejects.toThrow('Cannot reach the server. Is the backend running?')
  })

  it('explains a network failure as an unreachable server', async () => {
    mockFetch(new TypeError('Failed to fetch'))
    await expect(apiFetch('/api/presets')).rejects.toThrow('Cannot reach the server. Is the backend running?')
  })

  it('uses a generic message for other errors without JSON', async () => {
    mockFetch(new Response('oops', { status: 500 }))
    await expect(apiFetch('/api/presets')).rejects.toThrow('The server answered with an error (500).')
  })
})

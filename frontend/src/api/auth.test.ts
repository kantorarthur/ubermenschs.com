import { afterEach, describe, expect, it, vi } from 'vitest'
import { getMe } from './auth.ts'
import { getPreset } from './presets.ts'

function answer(status: number, body: unknown) {
  vi.stubGlobal(
    'fetch',
    vi.fn(() => Promise.resolve(new Response(JSON.stringify(body), { status }))),
  )
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('getMe', () => {
  it('returns the logged-in user', async () => {
    answer(200, { id: '1', email: 'ana@example.com', username: 'Ana' })
    await expect(getMe()).resolves.toEqual({ id: '1', email: 'ana@example.com', username: 'Ana' })
  })

  it('returns null for a visitor (401)', async () => {
    answer(401, { error: 'Log in to do this.' })
    await expect(getMe()).resolves.toBeNull()
  })

  it('still throws other errors', async () => {
    answer(503, { error: 'The database is not available right now.' })
    await expect(getMe()).rejects.toThrow('The database is not available right now.')
  })
})

describe('getPreset', () => {
  it('returns undefined for a preset that does not exist (404)', async () => {
    answer(404, { error: 'This preset does not exist.' })
    await expect(getPreset('42')).resolves.toBeUndefined()
  })

  it('still throws other errors', async () => {
    answer(401, { error: 'Log in to do this.' })
    await expect(getPreset('42')).rejects.toThrow('Log in to do this.')
  })
})

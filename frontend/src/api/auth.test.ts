import { afterEach, describe, expect, it, vi } from 'vitest'
import { changePassword, getMe } from './auth.ts'
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
    const ana = { id: '1', email: 'ana@example.com', username: 'Ana', createdAt: 1793900000 }
    answer(200, ana)
    await expect(getMe()).resolves.toEqual(ana)
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

describe('changePassword', () => {
  it('sends both passwords and passes on a wrong current password', async () => {
    answer(400, { error: 'The current password is wrong.' })
    await expect(changePassword({ currentPassword: 'old one', newPassword: 'a new password' })).rejects.toThrow(
      'The current password is wrong.',
    )
    const init = (fetch as unknown as { mock: { calls: [string, RequestInit][] } }).mock.calls[0]
    expect(init[0]).toBe('/api/auth/password')
    expect(JSON.parse(String(init[1].body))).toEqual({ currentPassword: 'old one', newPassword: 'a new password' })
  })
})

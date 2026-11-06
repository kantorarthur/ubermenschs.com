import { cleanup, render, screen } from '@testing-library/react'
import { MemoryRouter } from 'react-router'
import { afterEach, describe, expect, it, vi } from 'vitest'
import { AuthContext, type AuthContextValue } from '../context/authContext.ts'
import OverviewPage from './OverviewPage.tsx'

afterEach(() => {
  cleanup()
  vi.unstubAllGlobals()
})

function renderAs(user: AuthContextValue['user']) {
  const value: AuthContextValue = {
    user,
    isLoading: false,
    login: vi.fn(),
    register: vi.fn(),
    logout: vi.fn(),
  }
  render(
    <MemoryRouter>
      <AuthContext.Provider value={value}>
        <OverviewPage />
      </AuthContext.Provider>
    </MemoryRouter>,
  )
}

describe('OverviewPage', () => {
  it('invites visitors to register and links every section', () => {
    renderAs(null)
    expect(screen.getByRole('link', { name: 'Register' })).toBeTruthy()
    expect(screen.getByRole('link', { name: /Calendar/ }).getAttribute('href')).toBe('/calendar')
    expect(screen.getAllByRole('link', { name: /Reminder/ }).length).toBeGreaterThan(0)
  })

  it("shows a logged-in user's reminders for today", async () => {
    vi.stubGlobal(
      'fetch',
      vi.fn((url: string) => {
        const body = url.startsWith('/api/reminders')
          ? [{ id: '1', time: '23:59', message: 'Write down one thing you learned', enabled: true, timezone: 'UTC' }]
          : []
        return Promise.resolve(new Response(JSON.stringify(body), { status: 200 }))
      }),
    )
    renderAs({ id: '1', email: 'ana@example.com', username: 'Ana', createdAt: 1793900000 })
    expect(await screen.findByText('Write down one thing you learned')).toBeTruthy()
    expect(screen.getByText('Nothing in the calendar today.')).toBeTruthy()
  })
})

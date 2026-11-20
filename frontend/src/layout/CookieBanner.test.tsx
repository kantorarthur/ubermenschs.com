import { act, cleanup, fireEvent, render, screen } from '@testing-library/react'
import { MemoryRouter, Route, Routes, useNavigate } from 'react-router'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { AuthContext, type AuthContextValue } from '../context/authContext.ts'
import LoginPage from '../pages/LoginPage.tsx'
import { CONSENT_VERSION, reloadCookieConsent } from '../utils/cookieConsent.ts'
import CookieBanner from './CookieBanner.tsx'

const visitor: AuthContextValue = {
  user: null,
  isLoading: false,
  login: vi.fn(() => Promise.resolve()),
  register: vi.fn(() => Promise.resolve()),
  logout: vi.fn(() => Promise.resolve()),
  deleteAccount: vi.fn(() => Promise.resolve()),
}

// A link to change page inside the test, like clicking "Log in" in the header.
function GoTo({ path }: { path: string }) {
  const navigate = useNavigate()
  return (
    <button type="button" onClick={() => navigate(path)}>
      go {path}
    </button>
  )
}

function renderAt(path: string) {
  render(
    <MemoryRouter initialEntries={[path]}>
      <AuthContext.Provider value={visitor}>
        <Routes>
          <Route path="/" element={<GoTo path="/login" />} />
          <Route path="/login" element={<LoginPage />} />
        </Routes>
        <CookieBanner />
      </AuthContext.Provider>
    </MemoryRouter>,
  )
}

beforeEach(() => {
  localStorage.clear()
  reloadCookieConsent()
})

afterEach(() => {
  cleanup()
})

describe('CookieBanner', () => {
  it('asks a new visitor, links the privacy page, and Accept is remembered', () => {
    renderAt('/')
    expect(screen.getByRole('heading', { name: 'One cookie. No tracking.' })).toBeTruthy()
    expect(screen.getByRole('link', { name: 'Privacy' }).getAttribute('href')).toBe('/privacy')

    fireEvent.click(screen.getByRole('button', { name: 'Accept' }))
    expect(screen.queryByRole('heading', { name: 'One cookie. No tracking.' })).toBeNull()
    expect(localStorage.getItem('cookie-consent')).toBe(CONSENT_VERSION)

    // After a reload the answer is still there.
    act(() => reloadCookieConsent())
    expect(screen.queryByRole('heading', { name: 'One cookie. No tracking.' })).toBeNull()
  })

  it('"Not now" hides it until the login page, where it cannot be put away', () => {
    renderAt('/')
    fireEvent.click(screen.getByRole('button', { name: 'Not now' }))
    expect(screen.queryByRole('heading', { name: 'One cookie. No tracking.' })).toBeNull()

    fireEvent.click(screen.getByRole('button', { name: 'go /login' }))
    expect(screen.getByRole('heading', { name: 'Accept the cookie to log in' })).toBeTruthy()
    expect(screen.queryByRole('button', { name: 'Not now' })).toBeNull()
  })

  it('keeps the login form off until the cookie is accepted', () => {
    renderAt('/login')
    const logIn = screen.getByRole('button', { name: 'Log in' }) as HTMLButtonElement
    expect(logIn.disabled).toBe(true)
    expect(screen.getByText(/first accept the login cookie/)).toBeTruthy()

    fireEvent.click(screen.getByRole('button', { name: 'Accept' }))
    expect(logIn.disabled).toBe(false)
    expect(screen.queryByText(/first accept the login cookie/)).toBeNull()
  })

  it('asks again when the privacy page has a new version', () => {
    localStorage.setItem('cookie-consent', '2020-01')
    reloadCookieConsent()
    renderAt('/')
    expect(screen.getByRole('heading', { name: 'One cookie. No tracking.' })).toBeTruthy()
  })

  it('still works when the browser blocks localStorage', () => {
    const setItem = vi.spyOn(Storage.prototype, 'setItem').mockImplementation(() => {
      throw new Error('blocked')
    })
    renderAt('/')
    fireEvent.click(screen.getByRole('button', { name: 'Accept' }))
    expect(screen.queryByRole('heading', { name: 'One cookie. No tracking.' })).toBeNull()
    setItem.mockRestore()
  })
})

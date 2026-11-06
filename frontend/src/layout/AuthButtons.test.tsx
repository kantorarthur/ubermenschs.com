import { cleanup, fireEvent, render, screen } from '@testing-library/react'
import { MemoryRouter } from 'react-router'
import { afterEach, describe, expect, it, vi } from 'vitest'
import { AuthContext, type AuthContextValue } from '../context/authContext.ts'
import AuthButtons from './AuthButtons.tsx'

afterEach(cleanup)

function renderWith(user: AuthContextValue['user']) {
  const value: AuthContextValue = {
    user,
    isLoading: false,
    login: vi.fn(),
    register: vi.fn(),
    logout: vi.fn(() => Promise.resolve()),
  }
  render(
    <MemoryRouter>
      <AuthContext.Provider value={value}>
        <AuthButtons />
      </AuthContext.Provider>
    </MemoryRouter>,
  )
  return value
}

const ANA = { id: '1', email: 'ana@example.com', username: 'Ana', createdAt: 1793900000 }

describe('AuthButtons', () => {
  it('shows Log in and Register to visitors', () => {
    renderWith(null)
    expect(screen.getByText('Log in')).toBeTruthy()
    expect(screen.getByText('Register')).toBeTruthy()
  })

  it('opens the account menu when the username is clicked', () => {
    renderWith(ANA)
    const button = screen.getByRole('button', { name: /Ana/ })
    expect(button.getAttribute('aria-expanded')).toBe('false')
    expect(screen.queryByRole('menu')).toBeNull()

    fireEvent.click(button)
    expect(button.getAttribute('aria-expanded')).toBe('true')
    const items = screen.getAllByRole('menuitem').map((item) => item.textContent)
    expect(items).toEqual(['Account details', 'Account settings', 'Log out'])
    expect(screen.getByRole('menuitem', { name: 'Account details' }).getAttribute('href')).toBe('/account')
    expect(screen.getByRole('menuitem', { name: 'Account settings' }).getAttribute('href')).toBe('/account/settings')
  })

  it('closes with Escape or a click outside', () => {
    renderWith(ANA)
    fireEvent.click(screen.getByRole('button', { name: /Ana/ }))
    fireEvent.keyDown(document, { key: 'Escape' })
    expect(screen.queryByRole('menu')).toBeNull()

    fireEvent.click(screen.getByRole('button', { name: /Ana/ }))
    fireEvent.pointerDown(document.body)
    expect(screen.queryByRole('menu')).toBeNull()
  })

  it('logs out from the menu', () => {
    const value = renderWith(ANA)
    fireEvent.click(screen.getByRole('button', { name: /Ana/ }))
    fireEvent.click(screen.getByRole('menuitem', { name: 'Log out' }))
    expect(value.logout).toHaveBeenCalledOnce()
  })
})

import { cleanup, fireEvent, render, screen } from '@testing-library/react'
import { useState } from 'react'
import { MemoryRouter, Route, Routes } from 'react-router'
import { afterEach, describe, expect, it, vi } from 'vitest'
import { AuthContext, type AuthContextValue } from '../context/authContext.ts'
import AccountSettingsPage from './AccountSettingsPage.tsx'
import OverviewPage from './OverviewPage.tsx'

afterEach(() => {
  cleanup()
})

const ANA = { id: '1', email: 'ana@example.com', username: 'Ana', createdAt: 1793900000 }

// Like the real AuthProvider: after a successful delete nobody is logged in.
function Harness({ deleteOnServer }: { deleteOnServer: (password: string) => Promise<void> }) {
  const [user, setUser] = useState<AuthContextValue['user']>(ANA)
  const value: AuthContextValue = {
    user,
    isLoading: false,
    login: vi.fn(),
    register: vi.fn(),
    logout: vi.fn(),
    deleteAccount: async (password) => {
      await deleteOnServer(password)
      setUser(null)
    },
  }
  return (
    <AuthContext.Provider value={value}>
      <Routes>
        <Route path="/account/settings" element={<AccountSettingsPage />} />
        <Route path="/" element={<OverviewPage />} />
      </Routes>
    </AuthContext.Provider>
  )
}

function renderSettings(deleteOnServer: (password: string) => Promise<void>) {
  render(
    <MemoryRouter initialEntries={['/account/settings']}>
      <Harness deleteOnServer={deleteOnServer} />
    </MemoryRouter>,
  )
}

function fillIn(password: string, sure: boolean) {
  fireEvent.change(screen.getByLabelText('Your password'), { target: { value: password } })
  if (sure) {
    fireEvent.click(screen.getByLabelText('I understand that this cannot be undone.'))
  }
}

describe('Delete account', () => {
  it('needs the password and the checkbox before the button works', () => {
    renderSettings(vi.fn(() => Promise.resolve()))
    const button = screen.getByRole('button', { name: 'Delete my account' }) as HTMLButtonElement
    expect(button.disabled).toBe(true)

    fillIn('my password', false)
    expect(button.disabled).toBe(true)

    fireEvent.click(screen.getByLabelText('I understand that this cannot be undone.'))
    expect(button.disabled).toBe(false)
  })

  it('shows the error for a wrong password and stays on the page', async () => {
    const deleteOnServer = vi.fn(() => Promise.reject(new Error('The password is wrong.')))
    renderSettings(deleteOnServer)
    fillIn('wrong', true)
    fireEvent.click(screen.getByRole('button', { name: 'Delete my account' }))

    expect(await screen.findByText('The password is wrong.')).toBeTruthy()
    expect(deleteOnServer).toHaveBeenCalledWith('wrong')
    expect(screen.getByRole('heading', { name: 'Delete account' })).toBeTruthy()
    expect((screen.getByRole('button', { name: 'Delete my account' }) as HTMLButtonElement).disabled).toBe(false)
  })

  it('deletes the account and shows the overview for a visitor', async () => {
    const deleteOnServer = vi.fn(() => Promise.resolve())
    renderSettings(deleteOnServer)
    fillIn('my password', true)
    fireEvent.click(screen.getByRole('button', { name: 'Delete my account' }))

    expect(await screen.findByText('Your account and all its data were deleted.')).toBeTruthy()
    expect(deleteOnServer).toHaveBeenCalledWith('my password')
    expect(screen.getByRole('heading', { name: 'Overview' })).toBeTruthy()
    expect(screen.getByRole('link', { name: 'Register' })).toBeTruthy()
  })
})

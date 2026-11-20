import { cleanup, render, screen } from '@testing-library/react'
import { MemoryRouter } from 'react-router'
import { afterEach, describe, expect, it } from 'vitest'
import PrivacyPage from './PrivacyPage.tsx'

afterEach(() => {
  cleanup()
})

describe('PrivacyPage', () => {
  it('has every section, the contact and the way to delete the account', () => {
    render(
      <MemoryRouter>
        <PrivacyPage />
      </MemoryRouter>,
    )
    const headings = screen.getAllByRole('heading', { level: 2 }).map((h) => h.textContent)
    expect(headings).toEqual([
      'Who runs this site',
      'What is kept',
      'Cookies',
      'Why',
      'Where the data is',
      'How long',
      'Your rights',
      'Security',
      'Changes',
    ])
    expect(screen.getAllByRole('link', { name: 'kantorarthur222@gmail.com' })[0].getAttribute('href')).toBe(
      'mailto:kantorarthur222@gmail.com',
    )
    expect(screen.getByRole('link', { name: 'Account settings' }).getAttribute('href')).toBe('/account/settings')
  })
})

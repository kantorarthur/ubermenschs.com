import { cleanup, render, screen } from '@testing-library/react'
import { afterEach, describe, expect, it } from 'vitest'
import NotificationSettings from './NotificationSettings.tsx'

afterEach(cleanup)

describe('NotificationSettings', () => {
  it('explains what to do where the browser cannot get notifications', async () => {
    render(<NotificationSettings />)
    expect(await screen.findByText(/add the site to the home screen/)).toBeTruthy()
    expect(screen.queryByRole('button')).toBeNull()
  })
})

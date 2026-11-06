import { cleanup, fireEvent, render, screen } from '@testing-library/react'
import { afterEach, describe, expect, it } from 'vitest'
import InstallButton from './InstallButton.tsx'

afterEach(cleanup)

describe('InstallButton', () => {
  it('shows how to install when the browser has no install window', async () => {
    render(<InstallButton />)
    fireEvent.click(screen.getByRole('button', { name: /Download application/ }))
    expect(await screen.findByRole('dialog', { name: 'Install the app' })).toBeTruthy()

    fireEvent.click(screen.getByRole('button', { name: 'OK' }))
    expect(screen.queryByRole('dialog')).toBeNull()
  })
})

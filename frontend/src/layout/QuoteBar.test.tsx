import { cleanup, render, screen } from '@testing-library/react'
import { afterEach, describe, expect, it } from 'vitest'
import QuoteBar from './QuoteBar.tsx'

afterEach(cleanup)

describe('QuoteBar', () => {
  it('shows the quote in quotation marks and its author', () => {
    render(<QuoteBar quote={{ id: '1', text: 'Become who you are.', author: 'Friedrich Nietzsche' }} />)

    expect(screen.getByText('“Become who you are.”')).toBeTruthy()
    expect(screen.getByText('Friedrich Nietzsche')).toBeTruthy()
  })

  it('uses the smaller style for long quotes only', () => {
    const long = 'x'.repeat(91)
    const { rerender } = render(<QuoteBar quote={{ id: '1', text: 'Short.', author: 'A' }} />)
    const shortClasses = screen.getByText('“Short.”').className

    rerender(<QuoteBar quote={{ id: '2', text: long, author: 'A' }} />)
    const longClasses = screen.getByText(`“${long}”`).className

    expect(longClasses.split(' ').length).toBe(shortClasses.split(' ').length + 1)
  })

  it('shows nothing while there is no quote', () => {
    const { container } = render(<QuoteBar quote={undefined} />)
    expect(container.textContent).toBe('')
  })
})

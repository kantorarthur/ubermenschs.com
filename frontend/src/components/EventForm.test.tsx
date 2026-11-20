import { cleanup, fireEvent, render, screen } from '@testing-library/react'
import { afterEach, describe, expect, it, vi } from 'vitest'
import EventForm from './EventForm.tsx'

afterEach(cleanup)

describe('EventForm notifications', () => {
  it('sends every chosen notification, sorted', async () => {
    const onSave = vi.fn(() => Promise.resolve())
    render(<EventForm event={null} defaultDate="2026-11-07" onSave={onSave} onCancel={() => undefined} />)

    fireEvent.change(screen.getByLabelText('Title'), { target: { value: 'Exam' } })
    fireEvent.click(screen.getByLabelText('1 day before'))
    fireEvent.click(screen.getByLabelText('10 minutes before'))
    fireEvent.click(screen.getByRole('button', { name: 'Add event' }))

    await vi.waitFor(() => expect(onSave).toHaveBeenCalledOnce())
    expect(onSave).toHaveBeenCalledWith(expect.objectContaining({ title: 'Exam', notify: [10, 1440] }))
  })

  it('offers only the 09:00 choices for an all-day event, and keeps what still fits', async () => {
    const onSave = vi.fn(() => Promise.resolve())
    render(<EventForm event={null} defaultDate="2026-11-07" onSave={onSave} onCancel={() => undefined} />)

    fireEvent.change(screen.getByLabelText('Title'), { target: { value: 'Holiday' } })
    fireEvent.click(screen.getByLabelText('10 minutes before'))
    fireEvent.click(screen.getByLabelText('All day'))
    expect(screen.queryByLabelText('10 minutes before')).toBeNull()
    fireEvent.click(screen.getByLabelText('The day before, at 09:00'))
    fireEvent.click(screen.getByRole('button', { name: 'Add event' }))

    await vi.waitFor(() => expect(onSave).toHaveBeenCalledOnce())
    expect(onSave).toHaveBeenCalledWith(expect.objectContaining({ allDay: true, notify: [1440] }))
  })
})

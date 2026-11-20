import { cleanup, fireEvent, render, screen, within } from '@testing-library/react'
import { MemoryRouter } from 'react-router'
import { afterEach, describe, expect, it, vi } from 'vitest'
import { AuthContext, type AuthContextValue } from '../context/authContext.ts'
import { ALL_BOOKS } from '../data/studyPlan.ts'
import StudyPlanPage from './StudyPlanPage.tsx'

afterEach(() => {
  cleanup()
  vi.unstubAllGlobals()
})

const ANA = { id: '1', email: 'ana@example.com', username: 'Ana', createdAt: 1793900000 }

interface FakeServer {
  read: Set<string>
  reminders: { id: string; time: string; message: string; enabled: boolean; timezone: string }[]
  failNextPut?: boolean
  calls: { url: string; method: string; body: unknown }[]
}

// A small fake of the backend: /api/study and /api/reminders.
function fakeServer(read: string[] = [], reminderMessages: string[] = []): FakeServer {
  const server: FakeServer = {
    read: new Set(read),
    reminders: reminderMessages.map((message, i) => ({
      id: String(i + 1),
      time: '21:00',
      message,
      enabled: true,
      timezone: 'UTC',
    })),
    calls: [],
  }
  const json = (body: unknown, status = 200) => Promise.resolve(new Response(JSON.stringify(body), { status }))
  vi.stubGlobal(
    'fetch',
    vi.fn((url: string, init?: RequestInit) => {
      const method = init?.method ?? 'GET'
      const body = init?.body ? JSON.parse(String(init.body)) : undefined
      server.calls.push({ url, method, body })
      if (url === '/api/study') {
        return json({ read: [...server.read].sort() })
      }
      if (url.startsWith('/api/study/books/')) {
        if (server.failNextPut) {
          server.failNextPut = false
          return json({ error: 'Redis is not reachable.' }, 503)
        }
        const bookId = decodeURIComponent(url.slice('/api/study/books/'.length))
        if ((body as { read: boolean }).read) {
          server.read.add(bookId)
        } else {
          server.read.delete(bookId)
        }
        return json({ read: [...server.read].sort() })
      }
      if (url === '/api/reminders' && method === 'POST') {
        const reminder = { id: '99', enabled: true, ...(body as object) }
        return json(reminder, 201)
      }
      if (url === '/api/reminders') {
        return json(server.reminders)
      }
      return json({ error: 'not found' }, 404)
    }),
  )
  return server
}

function renderAs(user: AuthContextValue['user']) {
  const value: AuthContextValue = {
    user,
    isLoading: false,
    login: vi.fn(),
    register: vi.fn(),
    logout: vi.fn(),
    deleteAccount: vi.fn(),
  }
  render(
    <MemoryRouter>
      <AuthContext.Provider value={value}>
        <StudyPlanPage />
      </AuthContext.Provider>
    </MemoryRouter>,
  )
}

function readingNow() {
  return screen.getByRole('heading', { level: 2, name: /—/ }).textContent
}

describe('StudyPlanPage', () => {
  it('shows visitors the plan without ticks, and asks them to log in', () => {
    renderAs(null)
    expect(screen.getByText('Log in to tick the books you read and keep your progress.')).toBeTruthy()
    expect(screen.getByText('Meditations')).toBeTruthy()
    expect(screen.queryAllByRole('checkbox')).toHaveLength(0)
    expect(screen.queryByText(/Reading now/)).toBeNull()
  })

  it('starts with the first book and moves on after "Mark as read"', async () => {
    const server = fakeServer()
    renderAs(ANA)
    expect(await screen.findByText('Reading now · Stage 1')).toBeTruthy()
    expect(readingNow()).toBe('Marcus Aurelius — Meditations')
    expect(screen.getByText(`0 of ${ALL_BOOKS.length} books · Stage 1 of 5`)).toBeTruthy()

    fireEvent.click(screen.getByRole('button', { name: 'Mark as read' }))
    expect(await screen.findByText(`1 of ${ALL_BOOKS.length} books · Stage 1 of 5`)).toBeTruthy()
    expect(readingNow()).toBe('Epictetus — Enchiridion')
    expect(server.calls).toContainEqual({ url: '/api/study/books/meditations', method: 'PUT', body: { read: true } })
    expect((screen.getByRole('checkbox', { name: 'Meditations read' }) as HTMLInputElement).checked).toBe(true)
  })

  it('ticks and unticks books in the list', async () => {
    const server = fakeServer(['meditations'])
    renderAs(ANA)
    const enchiridion = (await screen.findByRole('checkbox', { name: 'Enchiridion read' })) as HTMLInputElement
    fireEvent.click(enchiridion)
    expect(await screen.findByText(`2 of ${ALL_BOOKS.length} books · Stage 1 of 5`)).toBeTruthy()

    fireEvent.click(screen.getByRole('checkbox', { name: 'Meditations read' }))
    expect(await screen.findByText(`1 of ${ALL_BOOKS.length} books · Stage 1 of 5`)).toBeTruthy()
    // The first unread book is "Reading now" again.
    expect(readingNow()).toBe('Marcus Aurelius — Meditations')
    expect([...server.read]).toEqual(['enchiridion'])
  })

  it('opens the stage of the next book', async () => {
    fakeServer(['meditations', 'enchiridion', 'letters-to-lucilius'])
    renderAs(ANA)
    expect(await screen.findByText('Reading now · Stage 2')).toBeTruthy()
    const greeks = screen.getByRole('button', { name: /The Greeks/ })
    const stoics = screen.getByRole('button', { name: /The Stoics/ })
    expect(greeks.getAttribute('aria-expanded')).toBe('true')
    expect(stoics.getAttribute('aria-expanded')).toBe('false')
    expect(within(greeks).getByText('0 / 3')).toBeTruthy()
    expect(within(stoics).getByText('3 / 3')).toBeTruthy()

    fireEvent.click(stoics)
    expect(stoics.getAttribute('aria-expanded')).toBe('true')
    expect(screen.getByRole('checkbox', { name: 'Meditations read' })).toBeTruthy()
  })

  it('keeps a stage open while its last book is ticked', async () => {
    fakeServer(['meditations', 'enchiridion'])
    renderAs(ANA)
    fireEvent.click(await screen.findByRole('checkbox', { name: 'Letters to Lucilius (a selection) read' }))
    expect(await screen.findByText('Reading now · Stage 2')).toBeTruthy()
    expect(screen.getByRole('button', { name: /The Stoics/ }).getAttribute('aria-expanded')).toBe('true')
    expect(screen.getByRole('button', { name: /The Greeks/ }).getAttribute('aria-expanded')).toBe('true')
  })

  it('adds a daily reading reminder once', async () => {
    const server = fakeServer()
    renderAs(ANA)
    fireEvent.click(await screen.findByRole('button', { name: 'Remind me at 21:00' }))
    expect(await screen.findByRole('button', { name: '✓ Daily reminder set' })).toBeTruthy()
    expect(screen.getByRole('status').textContent).toMatch(/A daily reminder at 21:00 was added/)
    const post = server.calls.find((c) => c.method === 'POST')
    expect(post?.url).toBe('/api/reminders')
    expect(post?.body).toMatchObject({ time: '21:00', message: 'Read 20 minutes of your philosophy book.' })
  })

  it('knows a reading reminder that already exists', async () => {
    fakeServer(['meditations'], ['Read 20 minutes of your philosophy book.'])
    renderAs(ANA)
    const button = (await screen.findByRole('button', { name: '✓ Daily reminder set' })) as HTMLButtonElement
    expect(button.disabled).toBe(true)
  })

  it('shows an error and keeps the progress when saving fails', async () => {
    const server = fakeServer()
    server.failNextPut = true
    renderAs(ANA)
    fireEvent.click(await screen.findByRole('button', { name: 'Mark as read' }))
    expect(await screen.findByText('Redis is not reachable.')).toBeTruthy()
    expect(readingNow()).toBe('Marcus Aurelius — Meditations')
  })

  it('celebrates the end of the plan', async () => {
    fakeServer(ALL_BOOKS.map(({ book }) => book.id))
    renderAs(ANA)
    expect(await screen.findByText('The plan is finished')).toBeTruthy()
    expect(screen.getByText(`${ALL_BOOKS.length} of ${ALL_BOOKS.length} books · finished`)).toBeTruthy()
    expect(screen.queryByRole('button', { name: 'Mark as read' })).toBeNull()
  })
})

describe('the study plan data', () => {
  it('has 15 books with ids the backend accepts, each used once', () => {
    const ids = ALL_BOOKS.map(({ book }) => book.id)
    expect(ids).toHaveLength(15)
    expect(new Set(ids).size).toBe(ids.length)
    for (const id of ids) {
      expect(id).toMatch(/^[a-z0-9-]{1,60}$/)
    }
  })
})

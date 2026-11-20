import { cleanup, fireEvent, render, screen } from '@testing-library/react'
import { MemoryRouter, Route, Routes } from 'react-router'
import { afterEach, describe, expect, it } from 'vitest'
import RichText from '../components/RichText.tsx'
import { STORIES } from '../data/stories.ts'
import StoriesPage from './StoriesPage.tsx'
import StoryPage from './StoryPage.tsx'

afterEach(() => {
  cleanup()
})

function renderAt(path: string) {
  render(
    <MemoryRouter initialEntries={[path]}>
      <Routes>
        <Route path="/stories" element={<StoriesPage />} />
        <Route path="/stories/:slug" element={<StoryPage />} />
      </Routes>
    </MemoryRouter>,
  )
}

describe('the stories', () => {
  it('lists every story in order and opens one', () => {
    renderAt('/stories')
    const links = screen.getAllByRole('link')
    expect(links.map((l) => l.getAttribute('href'))).toEqual([
      '/stories/what-is-the-ubermensch',
      '/stories/et-tu-brute',
      '/stories/alexander-and-diogenes',
    ])
    expect(screen.getByText('1. What is the Übermensch?')).toBeTruthy()

    fireEvent.click(screen.getByText('2. Et tu, Brute?!'))
    expect(screen.getByRole('heading', { level: 1, name: 'Et tu, Brute?!' })).toBeTruthy()
    expect(screen.getByText('“Et tu, Brute? Then fall, Caesar!”')).toBeTruthy()
    expect(screen.getByRole('heading', { level: 2, name: 'The Ides of March' })).toBeTruthy()
    expect(screen.getAllByRole('listitem')).toHaveLength(3)
  })

  it('shows the quotations, the commentary and the credits of the first story', () => {
    renderAt('/stories/what-is-the-ubermensch')
    expect(screen.getByRole('heading', { level: 1, name: 'What is the Übermensch?' })).toBeTruthy()
    expect(screen.getByText(/The quote about the sound of our steps will be added here/)).toBeTruthy()
    expect(screen.getAllByText('Nietzsche, Thus Spoke Zarathustra, Prologue 3')).toHaveLength(5)
    expect(screen.getAllByText('Commentary by Arthur Kantor')).toHaveLength(2)
    expect(screen.getByText(/translated by Thomas Common \(1909\)/).textContent).toMatch(/Commentary by Arthur Kantor\.$/)
    expect(screen.getByRole('link', { name: '← Philosophical stories' }).getAttribute('href')).toBe('/stories')
  })

  it('tells the meeting of Alexander and Diogenes', () => {
    renderAt('/stories/alexander-and-diogenes')
    expect(screen.getByRole('heading', { level: 1, name: 'Alexander and Diogenes' })).toBeTruthy()
    expect(screen.getAllByText('“Stand a little out of my sun.”')).toHaveLength(2)
    expect(screen.getByText(/Most people would say: what a fool Diogenes was/)).toBeTruthy()
    expect(screen.getAllByText('Commentary by Arthur Kantor')).toHaveLength(1)
    expect(screen.getByText(/Perrin, 1919/)).toBeTruthy()
  })

  it('says so for a story that does not exist', () => {
    renderAt('/stories/nothing-here')
    expect(screen.getByRole('heading', { level: 1, name: 'Story not found' })).toBeTruthy()
  })

  it('has unique slugs and numbers 1, 2, 3…, and no Superman left in the quotes', () => {
    expect(new Set(STORIES.map((s) => s.slug)).size).toBe(STORIES.length)
    expect(STORIES.map((s) => s.number)).toEqual(STORIES.map((_, i) => i + 1))
    for (const story of STORIES) {
      expect(story.slug).toMatch(/^[a-z0-9-]+$/)
      expect(JSON.stringify(story.sections)).not.toMatch(/Superman/)
    }
  })
})

describe('RichText', () => {
  it('turns *italic* and **bold** into tags and leaves HTML as text', () => {
    const { container } = render(<RichText text={'A **bold** and *italic* word, <b>not html</b>.'} />)
    expect(container.querySelector('strong')?.textContent).toBe('bold')
    expect(container.querySelector('em')?.textContent).toBe('italic')
    expect(container.querySelector('b')).toBeNull()
    expect(container.textContent).toBe('A bold and italic word, <b>not html</b>.')
  })
})

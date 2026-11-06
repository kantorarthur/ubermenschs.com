import type { Quote } from '../types/models.ts'

// The "Standard quotes" preset: fixed, shipped with the site.
// The quote in the header is picked at random from this list.
export const standardQuotes: Quote[] = [
  {
    id: 'std-1',
    text: 'Man is something to be overcome. What have you done to overcome him?',
    author: 'Friedrich Nietzsche',
  },
  {
    id: 'std-2',
    text: 'What is the ape to man? A laughingstock, a painful embarrassment. And that is just what man shall be to the Übermensch: a laughingstock, a painful embarrassment.',
    author: 'Friedrich Nietzsche',
  },
  {
    // Often attributed to Aristotle; the earliest known source is Elbert Hubbard.
    id: 'std-3',
    text: 'There is only one way to avoid criticism: do nothing, say nothing and be nothing.',
    author: 'Elbert Hubbard',
  },
  {
    // The site's own quote.
    id: 'std-4',
    text: 'There is only one thing standing between you and the Übermensch you: discipline.',
    author: 'ubermenschs.com',
  },
  {
    id: 'std-5',
    text: 'I am indebted to my father for living, but to my teacher for living well.',
    author: 'Alexander the Great',
  },
  {
    id: 'std-6',
    text: 'I would rather have a short life of glory than a long one of obscurity.',
    author: 'Alexander the Great',
  },
  {
    id: 'std-7',
    text: 'There is nothing impossible to him who will try.',
    author: 'Alexander the Great',
  },
  {
    id: 'std-8',
    text: 'The unexamined life is not worth living.',
    author: 'Socrates',
  },
]

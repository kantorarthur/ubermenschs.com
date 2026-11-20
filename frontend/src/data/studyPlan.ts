// The philosophical study plan: five stages, from the Stoics to the Übermensch, read in this order.
// A user's progress is only the ids of the books they marked as read (backend: user:{id}:study),
// so an id must never change once it is used: it is what the database remembers.
// Ids: 1-60 characters of a-z, 0-9 and "-" (the backend checks the same, src/study_store.cpp).

export interface StudyBook {
  id: string
  title: string
  author: string
  about: string
  duration: string
  // Shown under "Reading now", to think about while reading.
  question: string
}

export interface StudyStage {
  id: string
  title: string
  subtitle: string
  duration: string
  books: StudyBook[]
  exercise: { name: string; text: string }
}

export const STUDY_PLAN: StudyStage[] = [
  {
    id: 'stoics',
    title: 'The Stoics',
    subtitle: 'How to master your own mind',
    duration: '~2 months',
    books: [
      {
        id: 'meditations',
        title: 'Meditations',
        author: 'Marcus Aurelius',
        about: 'An emperor writing to himself. One passage a day.',
        duration: '~4 weeks',
        question: 'What is up to me today, and what is not?',
      },
      {
        id: 'enchiridion',
        title: 'Enchiridion',
        author: 'Epictetus',
        about: 'Short and hard: the handbook Marcus Aurelius learned from.',
        duration: '~2 weeks',
        question: 'Which of today’s worries were about things not up to me?',
      },
      {
        id: 'letters-to-lucilius',
        title: 'Letters to Lucilius (a selection)',
        author: 'Seneca',
        about: 'Time, fear, death and friendship, in letters to a friend.',
        duration: '~3 weeks',
        question: 'Where did my time go today, and did I choose it?',
      },
    ],
    exercise: {
      name: 'Evening review (Seneca)',
      text: 'Before sleep, ask yourself what you did badly today, what you did well, and what you can do better.',
    },
  },
  {
    id: 'greeks',
    title: 'The Greeks',
    subtitle: 'Where it all starts',
    duration: '~2 months',
    books: [
      {
        id: 'death-of-socrates',
        title: 'Apology, Crito and Phaedo',
        author: 'Plato',
        about: 'The trial and death of Socrates, and the examined life.',
        duration: '~3 weeks',
        question: 'Which of my beliefs have I never examined?',
      },
      {
        id: 'nicomachean-ethics',
        title: 'Nicomachean Ethics (books I–II)',
        author: 'Aristotle',
        about: 'Virtue as a habit you build, not a feeling you wait for.',
        duration: '~3 weeks',
        question: 'Which habit am I building without noticing?',
      },
      {
        id: 'letter-to-menoeceus',
        title: 'Letter to Menoeceus',
        author: 'Epicurus',
        about: 'The other answer: happiness as calm, without the fear of death.',
        duration: '~1 week',
        question: 'What do I want that would not make me calmer?',
      },
    ],
    exercise: {
      name: 'Socratic questioning',
      text: 'Take one thing you are sure of and ask: what exactly do I mean, and how do I know it?',
    },
  },
  {
    id: 'faith',
    title: 'Faith and the inner self',
    subtitle: 'Boethius, Augustine, Pascal',
    duration: '~2 months',
    books: [
      {
        id: 'consolation-of-philosophy',
        title: 'The Consolation of Philosophy',
        author: 'Boethius',
        about: 'A man waiting for his execution finds peace: the bridge from the Stoics to Christianity.',
        duration: '~3 weeks',
        question: 'What can no one take from me?',
      },
      {
        id: 'confessions',
        title: 'Confessions (books I–IX)',
        author: 'Augustine',
        about: 'A life told honestly, and a restless heart that finds its rest.',
        duration: '~4 weeks',
        question: 'What am I restless for?',
      },
      {
        id: 'pensees',
        title: 'Pensées (a selection)',
        author: 'Pascal',
        about: 'Notes on the greatness and the misery of man.',
        duration: '~2 weeks',
        question: 'What do I do so that I don’t have to think?',
      },
    ],
    exercise: {
      name: 'Pascal’s room',
      text: 'Sit quietly, alone, for 15 minutes: no phone, no music. Notice what you want to run to.',
    },
  },
  {
    id: 'meaning',
    title: 'The crisis of meaning',
    subtitle: 'Dostoevsky, Kierkegaard, Schopenhauer',
    duration: '~2 months',
    books: [
      {
        id: 'notes-from-underground',
        title: 'Notes from Underground',
        author: 'Dostoevsky',
        about: 'A man who refuses to be reasonable, and why that matters.',
        duration: '~2 weeks',
        question: 'Where do I act against what I know is good for me?',
      },
      {
        id: 'fear-and-trembling',
        title: 'Fear and Trembling',
        author: 'Kierkegaard',
        about: 'Abraham, faith, and a choice no rule can make for you.',
        duration: '~2 weeks',
        question: 'Which decision am I waiting for someone else to make?',
      },
      {
        id: 'wisdom-of-life',
        title: 'The Wisdom of Life',
        author: 'Schopenhauer',
        about: 'Practical advice from the great pessimist, the teacher Nietzsche later left behind.',
        duration: '~2 weeks',
        question: 'How much of my happiness depends on what others think of me?',
      },
    ],
    exercise: {
      name: 'The page you avoid',
      text: 'Write one page about a choice you keep putting off, and about what you fear in it.',
    },
  },
  {
    id: 'ubermensch',
    title: 'Übermensch',
    subtitle: 'Nietzsche: Twilight of the Idols → Thus Spoke Zarathustra',
    duration: '~3 months',
    books: [
      {
        id: 'twilight-of-the-idols',
        title: 'Twilight of the Idols',
        author: 'Nietzsche',
        about: 'The best way into Nietzsche: short, sharp, philosophy with a hammer.',
        duration: '~2 weeks',
        question: 'Which of my values did I choose, and which did I inherit?',
      },
      {
        id: 'beyond-good-and-evil',
        title: 'Beyond Good and Evil',
        author: 'Nietzsche',
        about: 'Against the philosophers’ certainties. If you want more after it: On the Genealogy of Morals.',
        duration: '~4 weeks',
        question: 'What would I still value if no one rewarded me for it?',
      },
      {
        id: 'zarathustra',
        title: 'Thus Spoke Zarathustra',
        author: 'Nietzsche',
        about: 'Where the Übermensch appears. Read it last, after everything before it.',
        duration: '~6 weeks',
        question: 'What would becoming more than I am today ask of me?',
      },
    ],
    exercise: {
      name: 'Eternal return',
      text: 'Imagine living this day again, exactly the same, forever. Would you want it? What would you change tomorrow?',
    },
  },
]

// Books to read alongside the plan or after it; not counted in the progress.
export const ALONG_THE_WAY = [
  { title: 'What Is Ancient Philosophy?', author: 'Pierre Hadot', about: 'Philosophy as a way of life, not a theory.' },
  { title: 'The Myth of Sisyphus', author: 'Albert Camus', about: 'One must imagine Sisyphus happy.' },
  { title: 'Man’s Search for Meaning', author: 'Viktor Frankl', about: 'Meaning found in the worst place on earth.' },
]

export const ALL_BOOKS: { book: StudyBook; stage: StudyStage; stageNumber: number }[] = STUDY_PLAN.flatMap(
  (stage, index) => stage.books.map((book) => ({ book, stage, stageNumber: index + 1 })),
)

// The first book of the plan that is not read yet; null when everything is read.
export function nextBook(read: ReadonlySet<string>) {
  return ALL_BOOKS.find(({ book }) => !read.has(book.id)) ?? null
}

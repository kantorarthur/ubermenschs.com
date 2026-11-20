// The philosophical stories: the list at /stories and each story at /stories/:slug.
//
// Text can use *italic* and **bold**; nothing else is interpreted (see components/RichText.tsx).
// Quotations must come from translations in the public domain (the source is named under each story).

export type StoryBlock =
  | { kind: 'paragraph'; text: string }
  // A quotation from the source, with where it comes from.
  | { kind: 'quote'; text: string; cite: string }
  // Arthur's own reading of the quotation above it; shown as "Commentary by Arthur Kantor".
  | { kind: 'commentary'; text: string }
  | { kind: 'list'; items: string[] }

export interface StorySection {
  heading: string
  blocks: StoryBlock[]
}

export interface Story {
  slug: string
  number: number
  title: string
  // Shown above the title, e.g. "Nietzsche" or "Rome, 44 BC".
  context: string
  summary: string
  minutesToRead: number
  // The quotation at the top. "pending" keeps its place until its exact words and source are known.
  epigraph: { text: string; cite: string } | { pending: string }
  intro?: string[]
  sections: StorySection[]
  closing: string
  credits: string
}

const ZARATHUSTRA = 'Nietzsche, Thus Spoke Zarathustra, Prologue 3'

export const STORIES: Story[] = [
  {
    slug: 'what-is-the-ubermensch',
    number: 1,
    title: 'What is the Übermensch?',
    context: 'Nietzsche',
    summary:
      'Zarathustra in the market place, the soul and the body as two brothers, and the sea that can take in a polluted stream.',
    minutesToRead: 6,
    epigraph: { pending: 'The quote about the sound of our steps will be added here, with a short note on what it means.' },
    sections: [
      {
        heading: 'Zarathustra comes down to the town',
        blocks: [
          {
            kind: 'paragraph',
            text: 'After leaving the old saint in the forest, Zarathustra reaches the town and speaks to the crowd in the market place:',
          },
          {
            kind: 'quote',
            text: 'I teach you the Übermensch. Man is something that is to be surpassed. What have ye done to surpass man?',
            cite: ZARATHUSTRA,
          },
          {
            kind: 'quote',
            text: 'What is the ape to man? A laughing-stock, a thing of shame. And just the same shall man be to the Übermensch.',
            cite: ZARATHUSTRA,
          },
          {
            kind: 'quote',
            text: 'The Übermensch is the meaning of the earth. Let your will say: The Übermensch shall be the meaning of the earth!',
            cite: ZARATHUSTRA,
          },
          {
            kind: 'paragraph',
            text: 'Man is not a goal but a **condition**, something to be overcome. The question is not *who* the Übermensch is, but what you did today to overcome yourself.',
          },
        ],
      },
      {
        heading: 'The soul and the body: two brothers',
        blocks: [
          {
            kind: 'quote',
            text: 'Once the soul looked contemptuously on the body… the soul wished the body meagre, ghastly, and famished. Oh, that soul was itself meagre, ghastly, and famished.',
            cite: ZARATHUSTRA,
          },
          {
            kind: 'commentary',
            text: 'It is not the soul that is to blame, but those who raised it. The soul is a child, and it has a brother: the body. When the parents feed and care for one brother and neglect the other, the neglected one does what any child does. It grows jealous and wants its brother to become like itself: meagre, ghastly and famished. Contempt for the body is the cry of a starved soul.',
          },
        ],
      },
      {
        heading: 'The polluted stream and the sea',
        blocks: [
          {
            kind: 'quote',
            text: 'Verily, a polluted stream is man. One must be a sea, to receive a polluted stream without becoming impure.',
            cite: ZARATHUSTRA,
          },
          {
            kind: 'commentary',
            text: 'Every morning, from the moment you wake, a wave of mud pours into you: the day that has begun. Your answer is a wave of clear water, the steps you take that day toward excellence and toward the condition of the Übermensch. The clear wave has to be far greater than the muddy one, so that by evening no trace of mud is left in the sea of that day.',
          },
          {
            kind: 'paragraph',
            text: 'There are two seas. **The sea of a day** is the road you walked that day. Each evening it flows into **the sea of your soul**, the sum of all your days, and it has been flowing there since your first day on earth.',
          },
        ],
      },
      {
        heading: 'Waking up late',
        blocks: [
          {
            kind: 'paragraph',
            text: 'If you wake up late and find the sea of your soul already full of mud, gathered over years, your *pothos* must be all the greater. *Pothos* is the Greek word for the burning longing that drove Alexander the Great, here a longing for excellence. The longer the mud has gathered, the harder it is to clear.',
          },
          {
            kind: 'paragraph',
            text: 'So every morning, from moment zero, do all you can to make the sea of the day as clear as possible. It is our duty as people whose condition is meant to be overcome.',
          },
        ],
      },
    ],
    closing: '“Lo, I teach you the Übermensch: he is that sea; in him can your great contempt be submerged.”',
    credits:
      'Quotations from *Thus Spoke Zarathustra*, translated by Thomas Common (1909), with “Übermensch” in place of “Superman”. Commentary by Arthur Kantor.',
  },
  {
    slug: 'et-tu-brute',
    number: 2,
    title: 'Et tu, Brute?!',
    context: 'Rome, 44 BC',
    summary:
      'How Gaius Julius Caesar was betrayed by the man he had pardoned, and what the daggers of the Ides of March bought.',
    minutesToRead: 7,
    epigraph: { text: 'Et tu, Brute? Then fall, Caesar!', cite: 'Shakespeare, *Julius Caesar*, Act III, Scene 1' },
    intro: [
      'The most famous last words in history were probably never spoken, at least not in Latin. Shakespeare wrote them in 1599, more than sixteen centuries after the murder. Suetonius, writing about 150 years after it, says Caesar died with only a groan, “though some have written that when Marcus Brutus rushed at him, he said in Greek, *‘You too, my child?’*”',
    ],
    sections: [
      {
        heading: 'The man who pardoned his enemies',
        blocks: [
          {
            kind: 'paragraph',
            text: 'In 49 BC Caesar crossed the Rubicon and won a civil war against Pompey. Marcus Junius Brutus fought on Pompey’s side. After the battle of Pharsalus, Caesar ordered that Brutus be spared. He pardoned him, made him governor of Cisalpine Gaul and then praetor of Rome. Brutus’s mother, Servilia, had been Caesar’s lover for years, and Caesar treated Brutus almost like a son. Mercy toward defeated enemies, *clementia*, was Caesar’s policy.',
          },
        ],
      },
      {
        heading: 'Dictator for life',
        blocks: [
          {
            kind: 'paragraph',
            text: 'In February 44 BC Caesar was named *dictator perpetuo*, dictator for life. At the festival of the Lupercalia, Mark Antony offered him a royal diadem. Caesar refused it, but Rome had seen the offer. Rome had driven out its last king in 509 BC, and the man who led that revolt was Brutus’s ancestor, Lucius Junius Brutus. To many senators, Caesar looked like a king in everything but the name.',
          },
        ],
      },
      {
        heading: 'The conspiracy',
        blocks: [
          {
            kind: 'paragraph',
            text: 'More than sixty senators joined the plot, led by Gaius Cassius Longinus. Cassius needed Brutus: his name could make a murder look like an act of liberty. Notes appeared on Brutus’s praetor’s seat: *“Brutus, you are asleep”*, *“You are no true Brutus.”* Brutus was the nephew and son-in-law of Cato, Caesar’s great Stoic opponent. He agreed.',
          },
        ],
      },
      {
        heading: 'The Ides of March',
        blocks: [
          { kind: 'paragraph', text: 'Caesar was warned more than once:' },
          {
            kind: 'list',
            items: [
              'The soothsayer Spurinna had told him to beware a danger that would come no later than the Ides of March. On the morning of the 15th, Caesar mocked him: the Ides had come. *“Aye, they have come,”* Spurinna answered, *“but they have not gone.”*',
              'The night before, his wife Calpurnia dreamed that she held him, stabbed, in her arms.',
              'On his way to the Senate, someone handed him a note revealing the plot. He kept it, unread, among the papers in his left hand.',
            ],
          },
          {
            kind: 'paragraph',
            text: 'He almost stayed home. Another Brutus, Decimus, one of the men Caesar trusted most and an heir in his will, persuaded him to go.',
          },
          {
            kind: 'paragraph',
            text: 'On 15 March 44 BC the Senate met in the hall of Pompey’s theatre. Tillius Cimber seized Caesar’s toga. *“Why, this is violence!”* Caesar cried. Casca struck first, and then the others. Caesar received twenty-three wounds. The physician Antistius later judged that only one of them, the second, in the chest, was fatal. According to Plutarch, when Caesar saw Brutus among the daggers, he stopped resisting and drew his toga over his face. He fell at the foot of the statue of Pompey, the rival he had defeated.',
          },
        ],
      },
      {
        heading: 'What the betrayal bought',
        blocks: [
          {
            kind: 'paragraph',
            text: 'The conspirators shouted for liberty, but Rome did not rise with them. Mark Antony’s funeral speech turned the crowd against the murderers, and a new civil war began. In 42 BC, at Philippi, Cassius and then Brutus took their own lives. Caesar’s adopted heir, Octavian, became Augustus, the first emperor. The men who killed Caesar to save the Republic were the ones who ended it.',
          },
          {
            kind: 'paragraph',
            text: 'Dante placed Brutus and Cassius at the very bottom of Hell, in the mouths of Lucifer, beside Judas.',
          },
        ],
      },
      {
        heading: 'The question that remains',
        blocks: [
          {
            kind: 'paragraph',
            text: '*“You too, my child?”* What wounded Caesar was not the dagger but the hand that held it: the man he had spared, raised and loved. For Brutus, it was the hardest choice of his life: his friend or his country. Was he a traitor, or the last free Roman? Every reader has to answer that alone.',
          },
        ],
      },
    ],
    closing: '“Kai su, teknon?” — You too, my child?',
    credits:
      'Sources: Suetonius, *Life of Julius Caesar* 80–82 (translated by J. C. Rolfe, 1914); Plutarch, *Life of Caesar* and *Life of Brutus*; Shakespeare, *Julius Caesar* (1599).',
  },
  {
    slug: 'alexander-and-diogenes',
    number: 3,
    title: 'Alexander and Diogenes',
    context: 'Corinth, 336 BC',
    summary:
      'The most powerful man in the world offers a beggar anything he wants, and the beggar asks only for his sunlight back.',
    minutesToRead: 5,
    epigraph: { text: 'Stand a little out of my sun.', cite: 'Diogenes to Alexander, in Plutarch, *Life of Alexander* 14' },
    sections: [
      {
        heading: 'The king and the dog',
        blocks: [
          {
            kind: 'paragraph',
            text: 'In 336 BC, Alexander of Macedon was twenty years old. His father had just been murdered, and at Corinth the Greek cities named him their leader for the war against Persia. Statesmen and philosophers came to congratulate him.',
          },
          {
            kind: 'paragraph',
            text: 'One man did not come. Diogenes of Sinope owned a cloak, a staff and a bag, and he slept in a large clay jar. He begged for his food, mocked the powerful, and once walked through the market in daylight with a lamp, “looking for a man”. The Athenians called him *the Dog*, and he took the name with pride: the school that followed him is still called the *Cynics*, from the Greek word for dog.',
          },
        ],
      },
      {
        heading: 'The meeting',
        blocks: [
          {
            kind: 'paragraph',
            text: 'So Alexander went to him. He found Diogenes in the Craneion, a cypress grove outside Corinth, lying in the sun. Surrounded by his companions, the king greeted him and asked whether he wanted anything. Diogenes raised himself a little, looked at him, and answered:',
          },
          { kind: 'quote', text: 'Stand a little out of my sun.', cite: 'Plutarch, Life of Alexander 14' },
          {
            kind: 'paragraph',
            text: 'Alexander’s companions laughed at the philosopher as they walked away. Alexander did not. Plutarch writes that he was struck by the pride and greatness of a man who looked down on him, and said:',
          },
          {
            kind: 'quote',
            text: 'But verily, if I were not Alexander, I would be Diogenes.',
            cite: 'Plutarch, Life of Alexander 14',
          },
        ],
      },
      {
        heading: 'What would you have asked for?',
        blocks: [
          {
            kind: 'commentary',
            text: 'Most people would say: what a fool Diogenes was. The master of the world stood in front of him, ready to give him anything, and he asked for nothing. But suppose he had asked for everything: gold, a palace, a city. In a few years none of it would have been left. Alexander’s own empire, the greatest the world had seen, did not outlive him: he died at thirty-two, and his generals tore it apart within a few years.',
          },
          {
            kind: 'paragraph',
            text: 'Everything that can be given can also be taken: by time, by thieves, by the next king. What Diogenes kept could not be taken, because it was never outside him. He needed nothing from Alexander, and that is exactly why, in that moment, he was the freer of the two. The conqueror of the world had to conquer in order to have; the beggar in the sun already had everything he needed.',
          },
          {
            kind: 'paragraph',
            text: 'Asking for nothing, he took the only thing that lasts. Alexander is remembered for what he took; Diogenes for what he refused. One sentence, said lying on the ground, has outlived the empire and is still told more than two thousand years later. It was the most valuable thing he could have done, worth more than anything of a physical nature he could have gained.',
          },
          {
            kind: 'paragraph',
            text: 'And Alexander understood it. *“If I were not Alexander, I would be Diogenes”*: the most powerful man alive admitted that the poorest one had something he could not conquer.',
          },
        ],
      },
    ],
    closing: '“If I were not Alexander, I would be Diogenes.”',
    credits:
      'Quotations from Plutarch, *Life of Alexander* 14 (translated by Bernadotte Perrin, 1919). Commentary by Arthur Kantor.',
  },
]

export function findStory(slug: string | undefined): Story | undefined {
  return STORIES.find((story) => story.slug === slug)
}

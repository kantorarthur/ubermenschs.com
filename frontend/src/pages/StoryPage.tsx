import { Link, useParams } from 'react-router'
import Page from '../components/Page.tsx'
import RichText from '../components/RichText.tsx'
import { findStory, type StoryBlock } from '../data/stories.ts'
import styles from './Stories.module.css'

// One story: /stories/:slug (the texts are in src/data/stories.ts).
export default function StoryPage() {
  const story = findStory(useParams().slug)

  if (!story) {
    return (
      <Page title="Story not found">
        <p>
          <Link to="/stories" className={styles.back}>
            ← Philosophical stories
          </Link>
        </p>
      </Page>
    )
  }

  return (
    <article className={styles.story}>
      <Link to="/stories" className={styles.back}>
        ← Philosophical stories
      </Link>
      <header>
        <p className={styles.eyebrow}>
          Story {story.number} · {story.context} · {story.minutesToRead} min read
        </p>
        <h1 className={styles.title}>{story.title}</h1>
      </header>

      <div className={styles.epigraph}>
        {'pending' in story.epigraph ? (
          <p className={styles.epigraphNote}>{story.epigraph.pending}</p>
        ) : (
          <>
            <p className={styles.epigraphText}>“{story.epigraph.text}”</p>
            <p className={styles.epigraphNote}>
              <RichText text={story.epigraph.cite} />
            </p>
          </>
        )}
      </div>

      {story.intro?.map((text) => (
        <p key={text}>
          <RichText text={text} />
        </p>
      ))}

      {story.sections.map((section) => (
        <section key={section.heading} className={styles.section}>
          <h2 className={styles.heading}>{section.heading}</h2>
          {section.blocks.map((block, index) => (
            <Block key={index} block={block} />
          ))}
        </section>
      ))}

      <p className={styles.closing}>{story.closing}</p>
      <p className={styles.credits}>
        <RichText text={story.credits} />
      </p>
    </article>
  )
}

function Block({ block }: { block: StoryBlock }) {
  switch (block.kind) {
    case 'paragraph':
      return (
        <p>
          <RichText text={block.text} />
        </p>
      )
    case 'quote':
      return (
        <blockquote className={styles.quote}>
          “{block.text}”<cite className={styles.cite}>{block.cite}</cite>
        </blockquote>
      )
    case 'commentary':
      return (
        <p>
          <span className={styles.label}>Commentary by Arthur Kantor</span> <RichText text={block.text} />
        </p>
      )
    case 'list':
      return (
        <ul className={styles.bullets}>
          {block.items.map((item) => (
            <li key={item}>
              <RichText text={item} />
            </li>
          ))}
        </ul>
      )
  }
}

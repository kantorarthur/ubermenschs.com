import { Link } from 'react-router'
import Page from '../components/Page.tsx'
import { STORIES } from '../data/stories.ts'
import styles from './Stories.module.css'

// "/stories": every philosophical story, in order.
export default function StoriesPage() {
  return (
    <Page title="Philosophical stories">
      <ul className={styles.list}>
        {STORIES.map((story) => (
          <li key={story.slug}>
            <Link to={`/stories/${story.slug}`} className={styles.card}>
              <span className={styles.meta}>
                Story {story.number} · {story.context}
              </span>
              <span className={styles.cardTitle}>
                {story.number}. {story.title}
              </span>
              <span className={styles.cardSummary}>{story.summary}</span>
            </Link>
          </li>
        ))}
      </ul>
    </Page>
  )
}

import type { Quote } from '../types/models.ts'
import styles from './QuoteBar.module.css'

interface QuoteBarProps {
  quote: Quote | undefined
}

// Long quotes get a smaller font so the header does not grow too tall.
const LONG_QUOTE_CHARACTERS = 90

// Only shows the quote it receives; AppShell decides which one.
export default function QuoteBar({ quote }: QuoteBarProps) {
  if (!quote) {
    return <div className={styles.bar} />
  }

  const isLong = quote.text.length > LONG_QUOTE_CHARACTERS

  return (
    <figure className={styles.bar}>
      <blockquote className={isLong ? `${styles.text} ${styles.long}` : styles.text}>
        “{quote.text}”
      </blockquote>
      <figcaption className={styles.author}>{quote.author}</figcaption>
    </figure>
  )
}

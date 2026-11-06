import { lovePassages } from '../data/loveQuotes.ts'
import styles from './LoveOneAnotherPage.module.css'

export default function LoveOneAnotherPage() {
  // The first passage (John 15:12) is the page's title verse; the rest follow as cards.
  const [first, ...rest] = lovePassages

  return (
    <section className={styles.page}>
      <figure className={styles.lead}>
        <blockquote className={styles.leadText}>{first.text}</blockquote>
        <figcaption className={styles.reference}>{first.reference}</figcaption>
      </figure>

      <ul className={styles.list}>
        {rest.map((passage) => (
          <li key={passage.id} className={styles.card}>
            <figure className={styles.figure}>
              <blockquote className={styles.text}>{passage.text}</blockquote>
              <figcaption className={styles.reference}>{passage.reference}</figcaption>
            </figure>
          </li>
        ))}
      </ul>

      <p className={styles.notice}>Scripture quotations: New International Version (NIV).</p>
    </section>
  )
}

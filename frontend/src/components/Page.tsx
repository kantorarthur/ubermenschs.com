import type { ReactNode } from 'react'
import styles from './Page.module.css'

interface PageProps {
  title: string
  subtitle?: string
  children?: ReactNode
}

// The common frame of every page in the content area: a title, an optional subtitle, the content.
export default function Page({ title, subtitle, children }: PageProps) {
  return (
    <section className={styles.page}>
      <header className={styles.heading}>
        <h1 className={styles.title}>{title}</h1>
        {subtitle && <p className={styles.subtitle}>{subtitle}</p>}
      </header>
      {children}
    </section>
  )
}

// A text card for pages that are not built yet.
export function ComingSoon({ children }: { children: ReactNode }) {
  return <p className={styles.card}>{children}</p>
}

import { Outlet, useLocation } from 'react-router'
import Logo from './Logo.tsx'
import QuoteBar from './QuoteBar.tsx'
import AuthButtons from './AuthButtons.tsx'
import Sidebar from './Sidebar.tsx'
import { landscapeBackgrounds, portraitBackgrounds } from '../data/backgrounds.ts'
import { standardQuotes } from '../data/standardQuotes.ts'
import { useMediaQuery } from '../hooks/useMediaQuery.ts'
import { useRandomOnChange } from '../hooks/useRandomOnChange.ts'
import styles from './AppShell.module.css'

// The frame around every page: background painting, header, sidebar, and the page itself.
export default function AppShell() {
  // location.key is new on every navigation, even when the same menu button is clicked again.
  const location = useLocation()
  const isPortrait = useMediaQuery('(orientation: portrait)')

  const backgrounds = isPortrait ? portraitBackgrounds : landscapeBackgrounds
  const background = useRandomOnChange(backgrounds, `${location.key}|${isPortrait}`)

  // Later: the quotes come from the user's active preset (Standard by default).
  const quote = useRandomOnChange(standardQuotes, location.key)

  return (
    <div className={styles.shell}>
      <div
        className={styles.background}
        style={background ? { backgroundImage: `url(${background})` } : undefined}
        aria-hidden="true"
      />
      <div className={styles.overlay} aria-hidden="true" />

      <header className={styles.header}>
        <Logo />
        <QuoteBar quote={quote} />
        <AuthButtons />
      </header>

      <div className={styles.body}>
        <Sidebar />
        <main className={styles.content}>
          <Outlet />
        </main>
      </div>
    </div>
  )
}

import { Outlet, useLocation } from 'react-router'
import Logo from './Logo.tsx'
import QuoteBar from './QuoteBar.tsx'
import AuthButtons from './AuthButtons.tsx'
import Sidebar from './Sidebar.tsx'
import { landscapeBackgrounds, portraitBackgrounds } from '../data/backgrounds.ts'
import { useQuotes } from '../context/quoteContext.ts'
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

  // The quotes come from the active preset (Standard by default).
  // `version` changes when the active preset or its quotes change, so the header picks again.
  const { headerQuotes, version } = useQuotes()
  const quote = useRandomOnChange(headerQuotes, `${location.key}|${version}`)

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

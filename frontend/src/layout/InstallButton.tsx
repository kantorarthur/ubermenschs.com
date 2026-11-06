import { useEffect, useRef, useState } from 'react'
import { installHelp, promptInstall, useInstallState, type InstallHelp } from '../utils/install.ts'
import styles from './InstallButton.module.css'

const HELP: Record<InstallHelp, string[]> = {
  ios: [
    'Tap the Share button (the square with an arrow) at the bottom of Safari.',
    'Choose “Add to Home Screen”, then “Add”.',
    'Open ubermenschs.com from its new icon. Notifications work only there.',
  ],
  'mac-safari': ['In the Safari menu bar, choose File → Add to Dock.', 'Then open ubermenschs.com from the Dock.'],
  firefox: [
    'Firefox on computers cannot install websites as apps.',
    'Open ubermenschs.com in Chrome or Edge and press this button there.',
  ],
  other: [
    'Open the browser’s menu (⋮ or …).',
    'Choose “Install app”, “Install ubermenschs.com” or “Add to Home screen”.',
  ],
}

// "Download application": installs the site as an app (its own icon and window), when it is not one yet.
export default function InstallButton() {
  const { installed, canPrompt } = useInstallState()
  const [help, setHelp] = useState<InstallHelp | null>(null)
  const container = useRef<HTMLDivElement>(null)

  // A click elsewhere or Escape closes the instructions.
  useEffect(() => {
    if (!help) {
      return
    }
    function onPointerDown(event: PointerEvent) {
      if (!container.current?.contains(event.target as Node)) {
        setHelp(null)
      }
    }
    function onKeyDown(event: KeyboardEvent) {
      if (event.key === 'Escape') {
        setHelp(null)
      }
    }
    document.addEventListener('pointerdown', onPointerDown)
    document.addEventListener('keydown', onKeyDown)
    return () => {
      document.removeEventListener('pointerdown', onPointerDown)
      document.removeEventListener('keydown', onKeyDown)
    }
  }, [help])

  if (installed) {
    return null
  }

  async function handleClick() {
    if (help) {
      setHelp(null)
      return
    }
    // Chrome, Edge, ...: the browser's own install window. Elsewhere: how to do it by hand.
    if (canPrompt && (await promptInstall())) {
      return
    }
    setHelp(installHelp(navigator.userAgent, navigator.maxTouchPoints))
  }

  return (
    <div className={styles.container} ref={container}>
      <button
        type="button"
        className={styles.button}
        onClick={() => void handleClick()}
        aria-expanded={help !== null}
        aria-controls="install-help"
      >
        <svg
          width="18"
          height="18"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          strokeWidth="1.8"
          strokeLinecap="round"
          strokeLinejoin="round"
          aria-hidden="true"
        >
          <path d="M12 4v11M7 10l5 5 5-5M5 20h14" />
        </svg>
        <span className={styles.label}>Download application</span>
      </button>

      {help && (
        <div id="install-help" role="dialog" aria-label="Install the app" className={styles.panel}>
          <p className={styles.title}>Install ubermenschs.com</p>
          <ol className={styles.steps}>
            {HELP[help].map((step) => (
              <li key={step}>{step}</li>
            ))}
          </ol>
          <button type="button" className={styles.close} onClick={() => setHelp(null)}>
            OK
          </button>
        </div>
      )}
    </div>
  )
}

import type { ReactNode } from 'react'
import { NavLink } from 'react-router'
import styles from './Sidebar.module.css'

interface MenuItem {
  to: string
  label: string
  icon: ReactNode
}

// Simple line icons, drawn with the current text color.
const iconProps = {
  width: 20,
  height: 20,
  viewBox: '0 0 24 24',
  fill: 'none',
  stroke: 'currentColor',
  strokeWidth: 1.8,
  strokeLinecap: 'round' as const,
  strokeLinejoin: 'round' as const,
  'aria-hidden': true,
}

const menu: MenuItem[] = [
  {
    to: '/quotes',
    label: 'Set quotes',
    icon: (
      <svg {...iconProps}>
        <path d="M7 7h4v4c0 3-2 5-4 6" />
        <path d="M15 7h4v4c0 3-2 5-4 6" />
      </svg>
    ),
  },
  {
    to: '/stories',
    label: 'Philosophical stories',
    icon: (
      <svg {...iconProps}>
        <path d="M4 5.5A2.5 2.5 0 0 1 6.5 3H20v16H6.5A2.5 2.5 0 0 0 4 21.5z" />
        <path d="M4 21.5V5.5" />
      </svg>
    ),
  },
  {
    to: '/love-one-another',
    label: 'Love each other as I have loved you.',
    icon: (
      <svg {...iconProps}>
        <path d="M12 20s-7-4.4-7-10a4 4 0 0 1 7-2.6A4 4 0 0 1 19 10c0 5.6-7 10-7 10z" />
      </svg>
    ),
  },
  {
    to: '/calendar',
    label: 'Calendar',
    icon: (
      <svg {...iconProps}>
        <rect x="3.5" y="5" width="17" height="15" rx="2" />
        <path d="M3.5 10h17M8 3v4M16 3v4" />
      </svg>
    ),
  },
  {
    to: '/reminders',
    label: 'Reminder',
    icon: (
      <svg {...iconProps}>
        <path d="M6 16V11a6 6 0 0 1 12 0v5l1.5 2h-15z" />
        <path d="M10 20.5a2 2 0 0 0 4 0" />
      </svg>
    ),
  },
  {
    to: '/study-plan',
    label: 'Philosophical study plan',
    icon: (
      <svg {...iconProps}>
        <path d="M9 6h11M9 12h11M9 18h11" />
        <path d="M4 6l1 1 2-2M4 12l1 1 2-2M4 18l1 1 2-2" />
      </svg>
    ),
  },
]

export default function Sidebar() {
  return (
    <nav aria-label="Main" className={styles.sidebar}>
      <ul className={styles.list}>
        {menu.map((item) => (
          <li key={item.to}>
            {/* NavLink knows when its URL is the current one, so it can be highlighted. */}
            <NavLink
              to={item.to}
              className={({ isActive }) => (isActive ? `${styles.link} ${styles.active}` : styles.link)}
            >
              {item.icon}
              <span>{item.label}</span>
            </NavLink>
          </li>
        ))}
      </ul>
    </nav>
  )
}

// Vite collects every .webp in these folders at build time.
// To add a background, drop a file into the folder: no code change needed.

const landscapeFiles = import.meta.glob<string>(
  '../assets/backgrounds/landscape/*.webp',
  { eager: true, import: 'default' },
)

const portraitFiles = import.meta.glob<string>(
  '../assets/backgrounds/portrait/*.webp',
  { eager: true, import: 'default' },
)

// Wide screens (laptop, desktop)
export const landscapeBackgrounds: string[] = Object.values(landscapeFiles)

// Tall screens (phones)
export const portraitBackgrounds: string[] = Object.values(portraitFiles)

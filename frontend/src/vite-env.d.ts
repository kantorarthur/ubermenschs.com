/// <reference types="vite/client" />

// Environment variables the frontend reads (they must start with VITE_).
interface ImportMetaEnv {
  // The backend's address in production, e.g. https://api.ubermenschs.com. Empty locally.
  readonly VITE_API_URL?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}

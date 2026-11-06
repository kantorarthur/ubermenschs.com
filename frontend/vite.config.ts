import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    // During development, requests to /api/... go to the C++ backend (docker compose up).
    // The browser sees one address, so no CORS setup is needed locally.
    proxy: {
      '/api': 'http://localhost:8080',
    },
  },
})

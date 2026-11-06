import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter } from 'react-router'
import './index.css'
import App from './App.tsx'
import { AuthProvider } from './context/AuthContext.tsx'
import { QuoteProvider } from './context/QuoteContext.tsx'
import { pushSupported, serviceWorker } from './utils/notifications.ts'

// The service worker shows the reminder notifications (public/sw.js).
if (pushSupported()) {
  serviceWorker().catch((error: unknown) => console.warn('The service worker could not be registered:', error))
}

// AuthProvider comes first: which quotes the header shows depends on who is logged in.
createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BrowserRouter>
      <AuthProvider>
        <QuoteProvider>
          <App />
        </QuoteProvider>
      </AuthProvider>
    </BrowserRouter>
  </StrictMode>,
)

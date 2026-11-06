import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter } from 'react-router'
import './index.css'
import App from './App.tsx'
import { QuoteProvider } from './context/QuoteContext.tsx'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BrowserRouter>
      <QuoteProvider>
        <App />
      </QuoteProvider>
    </BrowserRouter>
  </StrictMode>,
)

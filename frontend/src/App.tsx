import { Navigate, Route, Routes } from 'react-router'
import AppShell from './layout/AppShell.tsx'
import QuotesPage from './pages/QuotesPage.tsx'
import StandardQuotesPage from './pages/StandardQuotesPage.tsx'
import PresetPage from './pages/PresetPage.tsx'
import StoriesPage from './pages/StoriesPage.tsx'
import StoryPage from './pages/StoryPage.tsx'
import LoveOneAnotherPage from './pages/LoveOneAnotherPage.tsx'
import CalendarPage from './pages/CalendarPage.tsx'
import RemindersPage from './pages/RemindersPage.tsx'
import StudyPlanPage from './pages/StudyPlanPage.tsx'
import LoginPage from './pages/LoginPage.tsx'
import RegisterPage from './pages/RegisterPage.tsx'
import NotFoundPage from './pages/NotFoundPage.tsx'

// Every page renders inside AppShell (background, header, sidebar);
// React Router puts the page for the current URL where AppShell has <Outlet />.
export default function App() {
  return (
    <Routes>
      <Route element={<AppShell />}>
        <Route index element={<Navigate to="/quotes" replace />} />
        <Route path="quotes" element={<QuotesPage />} />
        <Route path="quotes/standard" element={<StandardQuotesPage />} />
        <Route path="quotes/:presetId" element={<PresetPage />} />
        <Route path="stories" element={<StoriesPage />} />
        <Route path="stories/:slug" element={<StoryPage />} />
        <Route path="love-one-another" element={<LoveOneAnotherPage />} />
        <Route path="calendar" element={<CalendarPage />} />
        <Route path="reminders" element={<RemindersPage />} />
        <Route path="study-plan" element={<StudyPlanPage />} />
        <Route path="login" element={<LoginPage />} />
        <Route path="register" element={<RegisterPage />} />
        <Route path="*" element={<NotFoundPage />} />
      </Route>
    </Routes>
  )
}

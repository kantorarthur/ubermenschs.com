import { createContext, useContext } from 'react'
import type { User } from '../types/models.ts'

export interface AuthContextValue {
  // The logged-in user, or null for a visitor.
  user: User | null
  // True until we know whether someone is logged in (first request after the page opens).
  isLoading: boolean
  login: (email: string, password: string) => Promise<void>
  register: (email: string, username: string, password: string) => Promise<void>
  logout: () => Promise<void>
}

export const AuthContext = createContext<AuthContextValue | null>(null)

// Lets any component know who is logged in, and log in or out.
export function useAuth(): AuthContextValue {
  const value = useContext(AuthContext)
  if (!value) {
    throw new Error('useAuth must be used inside <AuthProvider>.')
  }
  return value
}

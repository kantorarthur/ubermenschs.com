import { useCallback, useEffect, useMemo, useState, type ReactNode } from 'react'
import * as authApi from '../api/auth.ts'
import type { User } from '../types/models.ts'
import { forgetSubscriptionOnServer, syncSubscription } from '../utils/notifications.ts'
import { AuthContext, type AuthContextValue } from './authContext.ts'

// Knows who is logged in, for the whole app.
export function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<User | null>(null)
  const [isLoading, setIsLoading] = useState(true)

  // When the page opens, ask the backend whether the browser already has a valid login cookie.
  useEffect(() => {
    const request = new AbortController()
    authApi
      .getMe()
      .catch(() => null) // backend unreachable: treat as a visitor
      .then((me) => {
        if (!request.signal.aborted) {
          setUser(me)
          setIsLoading(false)
        }
      })
    return () => request.abort()
  }, [])

  // If this browser has notifications on, its reminders now belong to the account that logged in.
  // Best effort: a failure here must not stop the login.
  const login = useCallback(async (email: string, password: string) => {
    setUser(await authApi.login({ email, password }))
    syncSubscription().catch(() => undefined)
  }, [])

  const register = useCallback(async (email: string, username: string, password: string) => {
    setUser(await authApi.register({ email, username, password }))
    syncSubscription().catch(() => undefined)
  }, [])

  // After logging out, this browser no longer gets the account's reminders.
  const logout = useCallback(async () => {
    await forgetSubscriptionOnServer().catch(() => undefined)
    await authApi.logout()
    setUser(null)
  }, [])

  const value = useMemo<AuthContextValue>(
    () => ({ user, isLoading, login, register, logout }),
    [user, isLoading, login, register, logout],
  )

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

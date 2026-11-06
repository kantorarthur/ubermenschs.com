import type { User } from '../types/models.ts'
import { ApiError, apiFetch } from './client.ts'

// Accounts. The backend keeps the login in an HttpOnly cookie that JavaScript never sees;
// the browser sends it along with every request by itself.

export const PASSWORD_MIN = 10

// GET /api/auth/me  — the logged-in user, or null for a visitor
export async function getMe(): Promise<User | null> {
  try {
    return await apiFetch<User>('/api/auth/me')
  } catch (error) {
    if (error instanceof ApiError && error.status === 401) {
      return null
    }
    throw error
  }
}

// POST /api/auth/register  — creates the account and logs in
export function register(input: { email: string; username: string; password: string }): Promise<User> {
  return apiFetch<User>('/api/auth/register', { method: 'POST', body: input })
}

// POST /api/auth/login
export function login(input: { email: string; password: string }): Promise<User> {
  return apiFetch<User>('/api/auth/login', { method: 'POST', body: input })
}

// POST /api/auth/logout
export function logout(): Promise<void> {
  return apiFetch<void>('/api/auth/logout', { method: 'POST' })
}

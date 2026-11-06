// The one place that talks HTTP to the C++ backend.
//
// Locally the requests go to /api/... on the Vite server, which forwards them to
// http://localhost:8080 (see vite.config.ts). In production VITE_API_URL will hold
// the backend's address, e.g. https://api.ubermenschs.com.

const API_BASE = import.meta.env.VITE_API_URL ?? ''

const UNREACHABLE = 'Cannot reach the server. Is the backend running?'

// An HTTP error with the backend's message ({"error": "..."}) and the status code.
export class ApiError extends Error {
  readonly status: number

  constructor(status: number, message: string) {
    super(message)
    this.status = status
  }
}

async function readErrorMessage(response: Response): Promise<string> {
  try {
    const body = (await response.json()) as { error?: unknown }
    if (typeof body.error === 'string') {
      return body.error
    }
  } catch {
    // The body was not JSON; fall through to a generic message.
  }
  // 502 and 504 without our JSON come from a proxy (Vite locally) that could not reach the backend.
  if (response.status === 502 || response.status === 504) {
    return UNREACHABLE
  }
  return `The server answered with an error (${response.status}).`
}

// Sends a request and returns the JSON answer (or undefined for "204 No Content").
// Throws ApiError when the server answers with an error, and a plain Error when it cannot be reached.
export async function apiFetch<T>(path: string, options: { method?: string; body?: unknown } = {}): Promise<T> {
  let response: Response
  try {
    response = await fetch(`${API_BASE}${path}`, {
      method: options.method ?? 'GET',
      headers: options.body === undefined ? undefined : { 'Content-Type': 'application/json' },
      body: options.body === undefined ? undefined : JSON.stringify(options.body),
      // Sends the login cookie along once accounts exist (step 5).
      credentials: 'include',
    })
  } catch {
    throw new Error(UNREACHABLE)
  }

  if (!response.ok) {
    throw new ApiError(response.status, await readErrorMessage(response))
  }
  if (response.status === 204) {
    return undefined as T
  }
  return (await response.json()) as T
}

import { apiFetch } from './client.ts'

// Progress through the study plan (src/data/studyPlan.ts), stored by the C++ backend in Redis.

export interface StudyProgress {
  // Ids of the books marked as read.
  read: string[]
}

// GET /api/study
export function getStudyProgress(): Promise<StudyProgress> {
  return apiFetch<StudyProgress>('/api/study')
}

// PUT /api/study/books/:bookId   — answers with the whole progress after the change
export function setBookRead(bookId: string, read: boolean): Promise<StudyProgress> {
  return apiFetch<StudyProgress>(`/api/study/books/${encodeURIComponent(bookId)}`, {
    method: 'PUT',
    body: { read },
  })
}

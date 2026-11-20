import { Fragment, type ReactNode } from 'react'

// Text with *italic* and **bold**, for the stories (src/data/stories.ts).
// Only these two marks are interpreted; everything else, HTML included, stays plain text.
export default function RichText({ text }: { text: string }) {
  return <>{parse(text)}</>
}

const MARK = /(\*\*[^*]+\*\*|\*[^*]+\*)/g

function parse(text: string): ReactNode[] {
  return text.split(MARK).map((part, index) => {
    if (part.startsWith('**') && part.endsWith('**') && part.length > 4) {
      return <strong key={index}>{part.slice(2, -2)}</strong>
    }
    if (part.startsWith('*') && part.endsWith('*') && part.length > 2) {
      return <em key={index}>{part.slice(1, -1)}</em>
    }
    return <Fragment key={index}>{part}</Fragment>
  })
}

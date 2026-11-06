import { useParams } from 'react-router'
import Page, { ComingSoon } from '../components/Page.tsx'

// One story: /stories/:slug
export default function StoryPage() {
  const { slug } = useParams()

  return (
    <Page title="Philosophical story" subtitle={slug}>
      <ComingSoon>The story text will appear here.</ComingSoon>
    </Page>
  )
}

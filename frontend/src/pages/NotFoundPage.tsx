import { Link } from 'react-router'
import Page from '../components/Page.tsx'

export default function NotFoundPage() {
  return (
    <Page title="Page not found">
      <p>
        <Link to="/quotes">Back to the start</Link>
      </p>
    </Page>
  )
}

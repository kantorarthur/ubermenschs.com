import { useParams } from 'react-router'
import Page, { ComingSoon } from '../components/Page.tsx'

// One of the user's own presets: /quotes/:presetId
export default function PresetPage() {
  const { presetId } = useParams()

  return (
    <Page title="Quotes preset" subtitle={`Preset ${presetId}`}>
      <ComingSoon>Here you will add quotes to your own preset.</ComingSoon>
    </Page>
  )
}

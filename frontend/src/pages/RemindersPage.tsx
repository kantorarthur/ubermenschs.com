import Page, { ComingSoon } from '../components/Page.tsx'

export default function RemindersPage() {
  return (
    <Page title="Reminder" subtitle="A message delivered as a notification every day at the time you choose.">
      <ComingSoon>Your reminders will appear here.</ComingSoon>
    </Page>
  )
}

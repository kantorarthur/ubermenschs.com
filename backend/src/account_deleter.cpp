#include "account_deleter.hpp"

AccountDeleter::AccountDeleter(UserStore& users, SessionStore& sessions, PresetStore& presets, EventStore& events,
                               ReminderStore& reminders, PushStore& pushSubscriptions, StudyStore& study)
    : users_(users), sessions_(sessions), presets_(presets), events_(events), reminders_(reminders),
      pushSubscriptions_(pushSubscriptions), study_(study)
{
}

void AccountDeleter::deleteAccount(const std::string& userId)
{
    sessions_.removeAll(userId);
    pushSubscriptions_.removeAll(userId);
    reminders_.removeAll(userId);
    events_.removeAll(userId);
    presets_.removeAll(userId);
    study_.removeAll(userId);
    users_.remove(userId);
    // Once more: a login from another device during the steps above would have made a new session.
    sessions_.removeAll(userId);
}

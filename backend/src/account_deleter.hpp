#pragma once

#include "event_store.hpp"
#include "preset_store.hpp"
#include "push_store.hpp"
#include "reminder_store.hpp"
#include "user_store.hpp"

#include <string>

// Deletes an account and everything that belongs to it:
//
//   sessions        session:{fingerprint}, user:{id}:sessions
//   quote sets      preset:{id}, preset:{id}:quotes, quote:{id}, user:{id}:presets, user:{id}:settings
//   calendar        event:{id}, user:{id}:events, its members of events:due
//   reminders       reminder:{id}, user:{id}:reminders, its members of reminders:due
//   notifications   push:sub:{id}, user:{id}:push
//   the account     user:{id}, user:by-email:{email}
//
// The sessions go first, so the account cannot add anything new while the rest is being deleted, and
// once more at the end, in case someone logged in meanwhile. The account itself goes before that last
// step: if something fails halfway, the user can still log in and delete again.
class AccountDeleter
{
public:
    AccountDeleter(UserStore& users, SessionStore& sessions, PresetStore& presets, EventStore& events,
                   ReminderStore& reminders, PushStore& pushSubscriptions);

    void deleteAccount(const std::string& userId);

private:
    UserStore& users_;
    SessionStore& sessions_;
    PresetStore& presets_;
    EventStore& events_;
    ReminderStore& reminders_;
    PushStore& pushSubscriptions_;
};

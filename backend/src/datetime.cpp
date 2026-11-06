#include "datetime.hpp"

#include <chrono>
#include <cstddef>
#include <stdexcept>

namespace
{
    bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    // The number written by the digits text[first] .. text[first + count - 1];
    // -1 if one of them is not a digit.
    int number(const std::string& text, std::size_t first, std::size_t count)
    {
        int value = 0;
        for(std::size_t i = first; i < first + count; ++i)
        {
            if(!isDigit(text[i]))
                return -1;
            value = value * 10 + (text[i] - '0');
        }
        return value;
    }

    bool isDateAt(const std::string& text, std::size_t first)
    {
        if(text[first + 4] != '-' || text[first + 7] != '-')
            return false;

        const int year = number(text, first, 4);
        const int month = number(text, first + 5, 2);
        const int day = number(text, first + 8, 2);
        if(year < 1 || month < 1 || day < 1)
            return false;

        // year_month_day knows the length of every month, leap years included.
        const std::chrono::year_month_day date{std::chrono::year(year),
                                               std::chrono::month(static_cast<unsigned>(month)),
                                               std::chrono::day(static_cast<unsigned>(day))};
        return date.ok();
    }

    bool isTimeAt(const std::string& text, std::size_t first)
    {
        if(text[first + 2] != ':')
            return false;

        const int hour = number(text, first, 2);
        const int minute = number(text, first + 3, 2);
        return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
    }

    std::chrono::sys_days toDays(const std::string& date)
    {
        return std::chrono::sys_days{std::chrono::year_month_day{
            std::chrono::year(number(date, 0, 4)),
            std::chrono::month(static_cast<unsigned>(number(date, 5, 2))),
            std::chrono::day(static_cast<unsigned>(number(date, 8, 2)))}};
    }
}

bool isDate(const std::string& text)
{
    return text.size() == 10 && isDateAt(text, 0);
}

bool isLocalDateTime(const std::string& text)
{
    return text.size() == 16 && isDateAt(text, 0) && text[10] == 'T' && isTimeAt(text, 11);
}

bool isTimeOfDay(const std::string& text)
{
    return text.size() == 5 && isTimeAt(text, 0);
}

bool isTimeZone(const std::string& name)
{
    if(name.empty() || name.size() > 64)
        return false;

    try
    {
        std::chrono::locate_zone(name);
        return true;
    }
    catch(const std::runtime_error&)
    {
        return false;
    }
}

int daysBetween(const std::string& from, const std::string& to)
{
    return static_cast<int>((toDays(to) - toDays(from)).count());
}

long long dateTimeNumber(const std::string& dateTime)
{
    long long value = 0;
    for(const char c : dateTime)
    {
        if(isDigit(c))
            value = value * 10 + (c - '0');
    }
    return value;
}

std::chrono::sys_seconds nextReminderTime(const std::string& time, const std::string& timezone,
                                          std::chrono::sys_seconds after)
{
    using namespace std::chrono;

    const time_zone* zone = locate_zone(timezone);
    const minutes timeOfDay = hours(number(time, 0, 2)) + minutes(number(time, 3, 2));

    // Today at that time on the user's clock, or else tomorrow (or the day after, around a time change).
    const local_days today = floor<days>(zone->to_local(after));
    for(int day = 0; day <= 2; ++day)
    {
        const sys_seconds candidate = zone->to_sys(today + days(day) + timeOfDay, choose::earliest);
        if(candidate > after)
            return candidate;
    }
    throw std::runtime_error("no next reminder time found");
}

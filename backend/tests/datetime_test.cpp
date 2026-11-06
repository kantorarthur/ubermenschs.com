#include "datetime.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

TEST(DateTime, Dates)
{
    EXPECT_TRUE(isDate("2026-11-06"));
    EXPECT_TRUE(isDate("2028-02-29"));  // leap year
    EXPECT_FALSE(isDate("2026-02-29")); // not a leap year
    EXPECT_FALSE(isDate("2026-02-30"));
    EXPECT_FALSE(isDate("2026-13-01"));
    EXPECT_FALSE(isDate("2026-00-10"));
    EXPECT_FALSE(isDate("2026-1-6"));
    EXPECT_FALSE(isDate("2026/11/06"));
    EXPECT_FALSE(isDate("abcd-ef-gh"));
    EXPECT_FALSE(isDate(""));
}

TEST(DateTime, LocalDateTimes)
{
    EXPECT_TRUE(isLocalDateTime("2026-11-06T14:30"));
    EXPECT_TRUE(isLocalDateTime("2026-11-06T00:00"));
    EXPECT_TRUE(isLocalDateTime("2026-11-06T23:59"));
    EXPECT_FALSE(isLocalDateTime("2026-11-06T24:00"));
    EXPECT_FALSE(isLocalDateTime("2026-11-06T12:60"));
    EXPECT_FALSE(isLocalDateTime("2026-11-06 14:30"));
    EXPECT_FALSE(isLocalDateTime("2026-11-06T14:30:00"));
    EXPECT_FALSE(isLocalDateTime("2026-11-31T10:00"));
}

TEST(DateTime, TimesOfDay)
{
    EXPECT_TRUE(isTimeOfDay("07:30"));
    EXPECT_TRUE(isTimeOfDay("00:00"));
    EXPECT_TRUE(isTimeOfDay("23:59"));
    EXPECT_FALSE(isTimeOfDay("24:00"));
    EXPECT_FALSE(isTimeOfDay("7:30"));
    EXPECT_FALSE(isTimeOfDay("07:3x"));
    EXPECT_FALSE(isTimeOfDay(""));
}

TEST(DateTime, TimeZonesComeFromTheTzDatabase)
{
    EXPECT_TRUE(isTimeZone("Europe/Bucharest"));
    EXPECT_TRUE(isTimeZone("America/New_York"));
    EXPECT_TRUE(isTimeZone("UTC"));
    EXPECT_FALSE(isTimeZone("Europe/Atlantis"));
    EXPECT_FALSE(isTimeZone("../../etc/passwd"));
    EXPECT_FALSE(isTimeZone(""));
}

TEST(DateTime, DaysBetween)
{
    EXPECT_EQ(daysBetween("2026-11-06", "2026-11-06"), 0);
    EXPECT_EQ(daysBetween("2026-11-30", "2026-12-01"), 1);
    EXPECT_EQ(daysBetween("2028-02-28", "2028-03-01"), 2);
    EXPECT_EQ(daysBetween("2026-01-01", "2027-01-01"), 365);
    EXPECT_EQ(daysBetween("2026-11-06", "2026-11-01"), -5);
}

TEST(DateTime, NumbersSortLikeTheMoments)
{
    EXPECT_EQ(dateTimeNumber("2026-11-06T14:30"), 202611061430LL);
    EXPECT_LT(dateTimeNumber("2026-11-06T09:59"), dateTimeNumber("2026-11-06T10:00"));
    EXPECT_LT(dateTimeNumber("2026-12-31T23:59"), dateTimeNumber("2027-01-01T00:00"));
    // A double (the score in Redis) holds such numbers exactly.
    EXPECT_EQ(static_cast<long long>(static_cast<double>(dateTimeNumber("9999-12-31T23:59"))), 999912312359LL);
}

namespace
{
    using namespace std::chrono;

    // A moment in UTC, e.g. utc(2026y / 11 / 6, 5h + 30min).
    sys_seconds utc(year_month_day day, seconds timeOfDay)
    {
        return sys_days{day} + timeOfDay;
    }
}

TEST(NextReminderTime, LaterTodayOrTomorrow)
{
    // Bucharest is UTC+2 in winter: 07:30 there is 05:30 UTC.
    EXPECT_EQ(nextReminderTime("07:30", "Europe/Bucharest", utc(2026y / 11 / 6, 5h)), utc(2026y / 11 / 6, 5h + 30min));
    // Exactly at the time, or after it: tomorrow.
    EXPECT_EQ(nextReminderTime("07:30", "Europe/Bucharest", utc(2026y / 11 / 6, 5h + 30min)),
              utc(2026y / 11 / 7, 5h + 30min));
    EXPECT_EQ(nextReminderTime("07:30", "Europe/Bucharest", utc(2026y / 11 / 6, 20h)), utc(2026y / 11 / 7, 5h + 30min));
    // 00:00 local is 22:00 UTC the day before.
    EXPECT_EQ(nextReminderTime("00:00", "Europe/Bucharest", utc(2026y / 12 / 31, 21h)), utc(2026y / 12 / 31, 22h));
}

TEST(NextReminderTime, OtherTimeZones)
{
    // New York is UTC-5 in November.
    EXPECT_EQ(nextReminderTime("07:00", "America/New_York", utc(2026y / 11 / 6, 0h)), utc(2026y / 11 / 6, 12h));
    EXPECT_EQ(nextReminderTime("07:00", "UTC", utc(2026y / 11 / 6, 0h)), utc(2026y / 11 / 6, 7h));
    // 23:30 in New York is already the next day in UTC.
    EXPECT_EQ(nextReminderTime("23:30", "America/New_York", utc(2026y / 11 / 6, 12h)), utc(2026y / 11 / 7, 4h + 30min));
}

TEST(NextReminderTime, SummerTimeStarts)
{
    // On 28 March 2027 the clocks in Bucharest jump from 03:00 to 04:00, so 03:30 does not exist.
    // The reminder goes off at the jump: 03:00 winter time = 01:00 UTC.
    EXPECT_EQ(nextReminderTime("03:30", "Europe/Bucharest", utc(2027y / 3 / 27, 12h)), utc(2027y / 3 / 28, 1h));
    // The day after, 03:30 summer time (UTC+3) = 00:30 UTC.
    EXPECT_EQ(nextReminderTime("03:30", "Europe/Bucharest", utc(2027y / 3 / 28, 1h)), utc(2027y / 3 / 29, 30min));
    // A normal time that day moves by one hour in UTC: 07:00 summer time = 04:00 UTC.
    EXPECT_EQ(nextReminderTime("07:00", "Europe/Bucharest", utc(2027y / 3 / 27, 12h)), utc(2027y / 3 / 28, 4h));
}

TEST(NextReminderTime, SummerTimeEndsAndTheHourHappensTwice)
{
    // On 31 October 2027 the clocks go back from 04:00 to 03:00: 03:30 happens twice.
    // The first one (summer time, UTC+3 = 00:30 UTC) counts...
    EXPECT_EQ(nextReminderTime("03:30", "Europe/Bucharest", utc(2027y / 10 / 30, 12h)), utc(2027y / 10 / 31, 30min));
    // ...and the second one (01:30 UTC) does not: the next is the day after, in winter time.
    EXPECT_EQ(nextReminderTime("03:30", "Europe/Bucharest", utc(2027y / 10 / 31, 30min)), utc(2027y / 11 / 1, 1h + 30min));
}

TEST(NextReminderTime, UnknownTimeZone)
{
    EXPECT_THROW(nextReminderTime("07:00", "Europe/Atlantis", utc(2026y / 11 / 6, 0h)), std::runtime_error);
}

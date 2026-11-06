#include "datetime.hpp"

#include <gtest/gtest.h>

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

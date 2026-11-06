#include "errors.hpp"
#include "text.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(Trim, RemovesSpacesTabsAndLineBreaksAtBothEnds)
{
    EXPECT_EQ(trim("  hello \t\n"), "hello");
    EXPECT_EQ(trim("a b"), "a b");
}

TEST(Trim, OnlySpacesBecomeEmpty)
{
    EXPECT_EQ(trim("   "), "");
    EXPECT_EQ(trim(""), "");
}

TEST(Utf8Length, CountsCharactersNotBytes)
{
    EXPECT_EQ(utf8Length("abc"), 3u);
    EXPECT_EQ(utf8Length("Übermensch"), 10u); // Ü takes 2 bytes
    EXPECT_EQ(utf8Length("ăîșțâ"), 5u);       // each takes 2 bytes
    EXPECT_EQ(utf8Length("“”"), 2u);          // each takes 3 bytes
    EXPECT_EQ(utf8Length(""), 0u);
}

TEST(NormalizeEmail, TrimsAndLowersLetters)
{
    EXPECT_EQ(normalizeEmail("  Ana@Example.COM "), "ana@example.com");
}

TEST(IsId, AcceptsOnlyDigits)
{
    EXPECT_TRUE(isId("1"));
    EXPECT_TRUE(isId("1234567890"));
    EXPECT_FALSE(isId(""));
    EXPECT_FALSE(isId("12a"));
    EXPECT_FALSE(isId("-1"));
    EXPECT_FALSE(isId("../etc"));
    EXPECT_FALSE(isId("1234567890123456789")); // 19 digits: longer than any id we create
}

TEST(RequireText, ReturnsTheTrimmedText)
{
    EXPECT_EQ(requireText("  Stoics ", 60, "empty", "long"), "Stoics");
}

TEST(RequireText, ThrowsWithTheGivenMessages)
{
    try
    {
        requireText("   ", 60, "empty message", "long message");
        FAIL() << "an empty text must be refused";
    }
    catch(const ValidationError& error)
    {
        EXPECT_STREQ(error.what(), "empty message");
    }

    try
    {
        requireText(std::string(61, 'x'), 60, "empty message", "long message");
        FAIL() << "a text over the limit must be refused";
    }
    catch(const ValidationError& error)
    {
        EXPECT_STREQ(error.what(), "long message");
    }
}

TEST(RequireText, TheLimitCountsCharacters)
{
    std::string sixtyLetters;
    for(int i = 0; i < 60; ++i)
        sixtyLetters += "ș"; // 60 characters, 120 bytes

    EXPECT_NO_THROW(requireText(sixtyLetters, 60, "empty", "long"));
    EXPECT_THROW(requireText(sixtyLetters + "ș", 60, "empty", "long"), ValidationError);
}

#include "crypto.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
    bool isLowerHex(const std::string& text)
    {
        return std::all_of(text.begin(), text.end(), [](unsigned char c)
        {
            return std::isdigit(c) != 0 || (c >= 'a' && c <= 'f');
        });
    }

    // libsodium must be initialised once before the tests in this file.
    class CryptoTest : public testing::Test
    {
    protected:
        static void SetUpTestSuite()
        {
            ASSERT_TRUE(initCrypto());
        }
    };
}

TEST_F(CryptoTest, HashIsArgon2idAndNotThePassword)
{
    const std::string hash = hashPassword("correct horse battery");
    EXPECT_EQ(hash.rfind("$argon2id$", 0), 0u);
    EXPECT_EQ(hash.find("correct horse battery"), std::string::npos);
}

TEST_F(CryptoTest, VerifyAcceptsOnlyTheRightPassword)
{
    const std::string hash = hashPassword("correct horse battery");
    EXPECT_TRUE(verifyPassword(hash, "correct horse battery"));
    EXPECT_FALSE(verifyPassword(hash, "correct horse batterY"));
    EXPECT_FALSE(verifyPassword(hash, ""));
}

TEST_F(CryptoTest, SamePasswordGivesDifferentHashes)
{
    // Each hash has its own random salt, so equal passwords cannot be spotted in the database.
    EXPECT_NE(hashPassword("same password"), hashPassword("same password"));
}

TEST_F(CryptoTest, RandomTokensAre64HexCharactersAndDiffer)
{
    const std::string first = randomToken();
    const std::string second = randomToken();
    EXPECT_EQ(first.size(), 64u);
    EXPECT_TRUE(isLowerHex(first));
    EXPECT_NE(first, second);
}

TEST_F(CryptoTest, FingerprintIsStableAndDifferentFromTheToken)
{
    const std::string token = randomToken();
    EXPECT_EQ(fingerprint(token), fingerprint(token));
    EXPECT_NE(fingerprint(token), token);
    EXPECT_NE(fingerprint(token), fingerprint(randomToken()));
    EXPECT_EQ(fingerprint(token).size(), 64u);
}

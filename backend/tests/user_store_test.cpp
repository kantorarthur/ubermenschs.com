#include "errors.hpp"
#include "redis_fixture.hpp"
#include "user_store.hpp"

#include <optional>
#include <string>

namespace
{
    const std::string PASSWORD = "a long enough password";

    class UserStoreTest : public RedisTest
    {
    };
}

TEST_F(UserStoreTest, RegisterSavesTheAccountWithANormalizedEmail)
{
    UserStore users(*redis);
    const User user = users.registerUser("  Ana@Example.com ", " Ana ", PASSWORD);

    EXPECT_EQ(user.email, "ana@example.com");
    EXPECT_EQ(user.username, "Ana");
    EXPECT_EQ(redis->get("user:by-email:ana@example.com").value(), user.id);

    // Only the hash is stored, never the password.
    const auto hash = redis->hget("user:" + user.id, "passwordHash");
    ASSERT_TRUE(hash);
    EXPECT_EQ(hash->rfind("$argon2id$", 0), 0u);
    EXPECT_EQ(hash->find(PASSWORD), std::string::npos);
}

TEST_F(UserStoreTest, AnEmailCanHaveOnlyOneAccount)
{
    UserStore users(*redis);
    users.registerUser("ana@example.com", "Ana", PASSWORD);
    EXPECT_THROW(users.registerUser("ANA@example.com", "Other", PASSWORD), ConflictError);
}

TEST_F(UserStoreTest, BadDataIsRefused)
{
    UserStore users(*redis);
    EXPECT_THROW(users.registerUser("", "Ana", PASSWORD), ValidationError);
    EXPECT_THROW(users.registerUser("not-an-email", "Ana", PASSWORD), ValidationError);
    EXPECT_THROW(users.registerUser("a@b", "Ana", PASSWORD), ValidationError);
    EXPECT_THROW(users.registerUser("a b@example.com", "Ana", PASSWORD), ValidationError);
    EXPECT_THROW(users.registerUser("ana@example.com", "A", PASSWORD), ValidationError);
    EXPECT_THROW(users.registerUser("ana@example.com", std::string(31, 'a'), PASSWORD), ValidationError);
    EXPECT_THROW(users.registerUser("ana@example.com", "Ana", "short"), ValidationError);
    EXPECT_THROW(users.registerUser("ana@example.com", "Ana", "123456789"), ValidationError); // 9 characters

    // None of the refused attempts may have claimed the email.
    EXPECT_FALSE(redis->exists("user:by-email:ana@example.com"));
}

TEST_F(UserStoreTest, AuthenticateNeedsTheRightPassword)
{
    UserStore users(*redis);
    const User ana = users.registerUser("ana@example.com", "Ana", PASSWORD);

    const std::optional<User> loggedIn = users.authenticate(" ANA@example.com", PASSWORD);
    ASSERT_TRUE(loggedIn);
    EXPECT_EQ(loggedIn->id, ana.id);

    EXPECT_FALSE(users.authenticate("ana@example.com", "a wrong password!"));
    EXPECT_FALSE(users.authenticate("nobody@example.com", PASSWORD));
}

TEST_F(UserStoreTest, FindById)
{
    UserStore users(*redis);
    const User ana = users.registerUser("ana@example.com", "Ana", PASSWORD);

    const std::optional<User> found = users.findById(ana.id);
    ASSERT_TRUE(found);
    EXPECT_EQ(found->username, "Ana");
    EXPECT_FALSE(users.findById("999"));
}

TEST_F(UserStoreTest, AnEmailFromAnUnfinishedRegistrationCanBeUsedAgain)
{
    // An earlier registration claimed the email but stopped before writing the account.
    redis->set("user:by-email:ana@example.com", "999");

    UserStore users(*redis);
    const User ana = users.registerUser("ana@example.com", "Ana", PASSWORD);
    EXPECT_EQ(redis->get("user:by-email:ana@example.com").value(), ana.id);
}

TEST_F(UserStoreTest, RegisterRemembersWhenTheAccountWasMade)
{
    UserStore users(*redis);
    const User user = users.registerUser("ana@example.com", "Ana", "correct horse battery");
    EXPECT_GT(user.createdAt, 1700000000); // a real moment, not 0
    EXPECT_EQ(users.findById(user.id)->createdAt, user.createdAt);
}

TEST_F(UserStoreTest, ChangePassword)
{
    UserStore users(*redis);
    const User user = users.registerUser("ana@example.com", "Ana", "correct horse battery");

    EXPECT_THROW(users.changePassword(user.id, "wrong password", "a brand new password"), UnauthorizedError);
    EXPECT_THROW(users.changePassword(user.id, "correct horse battery", "short"), ValidationError);
    // Nothing changed so far.
    EXPECT_TRUE(users.authenticate("ana@example.com", "correct horse battery"));

    users.changePassword(user.id, "correct horse battery", "a brand new password");
    EXPECT_FALSE(users.authenticate("ana@example.com", "correct horse battery"));
    EXPECT_TRUE(users.authenticate("ana@example.com", "a brand new password"));
    EXPECT_THROW(users.changePassword("999", "anything at all", "a brand new password"), UnauthorizedError);
}

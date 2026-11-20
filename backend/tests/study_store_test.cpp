#include "errors.hpp"
#include "redis_fixture.hpp"
#include "study_store.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace
{
    class StudyStoreTest : public RedisTest
    {
    protected:
        void SetUp() override
        {
            RedisTest::SetUp();
            if(IsSkipped())
                return;
            store = std::make_unique<StudyStore>(*redis);
        }

        std::unique_ptr<StudyStore> store;
    };

    using Books = std::vector<std::string>;
}

TEST_F(StudyStoreTest, NothingIsReadAtFirst)
{
    EXPECT_TRUE(store->readBooks("1").empty());
}

TEST_F(StudyStoreTest, MarksBooksAsReadAndNotRead)
{
    store->setRead("1", "meditations", true);
    store->setRead("1", "enchiridion", true);
    EXPECT_EQ(store->readBooks("1"), (Books{"enchiridion", "meditations"}));

    store->setRead("1", "meditations", false);
    EXPECT_EQ(store->readBooks("1"), Books{"enchiridion"});
}

TEST_F(StudyStoreTest, MarkingTwiceChangesNothing)
{
    store->setRead("1", "meditations", true);
    store->setRead("1", "meditations", true);
    store->setRead("1", "letters-to-lucilius", false);
    EXPECT_EQ(store->readBooks("1"), Books{"meditations"});
}

TEST_F(StudyStoreTest, EveryUserHasTheirOwnProgress)
{
    store->setRead("1", "meditations", true);
    store->setRead("2", "zarathustra", true);
    EXPECT_EQ(store->readBooks("1"), Books{"meditations"});
    EXPECT_EQ(store->readBooks("2"), Books{"zarathustra"});
}

TEST_F(StudyStoreTest, RejectsIdsThatAreNotBookIds)
{
    for(const std::string& bookId : {std::string(""), std::string("Meditations"), std::string("a b"),
                                     std::string("../x"), std::string("book:1"), std::string(61, 'a')})
        EXPECT_THROW(store->setRead("1", bookId, true), ValidationError) << bookId;
    EXPECT_NO_THROW(store->setRead("1", std::string(60, 'a'), true));
    EXPECT_EQ(store->readBooks("1").size(), 1u);
}

TEST_F(StudyStoreTest, HasALimitButAlreadyReadBooksStillWork)
{
    for(std::size_t i = 0; i < limits::studyBooksRead; ++i)
        store->setRead("1", "book-" + std::to_string(i), true);
    EXPECT_THROW(store->setRead("1", "one-more", true), ValidationError);
    EXPECT_NO_THROW(store->setRead("1", "book-0", true));
    EXPECT_NO_THROW(store->setRead("1", "book-0", false));
    EXPECT_NO_THROW(store->setRead("1", "one-more", true));
}

TEST_F(StudyStoreTest, RemoveAllForgetsTheProgress)
{
    store->setRead("1", "meditations", true);
    store->setRead("2", "meditations", true);
    store->removeAll("1");
    EXPECT_TRUE(store->readBooks("1").empty());
    EXPECT_FALSE(redis->exists("user:1:study"));
    EXPECT_EQ(store->readBooks("2"), Books{"meditations"});
}

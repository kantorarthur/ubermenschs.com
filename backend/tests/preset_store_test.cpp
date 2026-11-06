#include "errors.hpp"
#include "preset_store.hpp"
#include "redis_fixture.hpp"

#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace
{
    const std::string ANA = "1";
    const std::string BOB = "2";

    class PresetStoreTest : public RedisTest
    {
    };
}

TEST_F(PresetStoreTest, CreateListAndFind)
{
    PresetStore store(*redis);
    const Preset stoics = store.create(ANA, "  Stoics ");
    store.create(ANA, "Nietzsche");

    EXPECT_EQ(stoics.name, "Stoics");
    const std::vector<Preset> presets = store.list(ANA);
    ASSERT_EQ(presets.size(), 2u);
    EXPECT_EQ(presets[0].name, "Stoics"); // in the order they were created
    EXPECT_EQ(presets[1].name, "Nietzsche");

    const std::optional<Preset> found = store.find(ANA, stoics.id);
    ASSERT_TRUE(found);
    EXPECT_TRUE(found->quotes.empty());
}

TEST_F(PresetStoreTest, NameRules)
{
    PresetStore store(*redis);
    EXPECT_THROW(store.create(ANA, "   "), ValidationError);
    EXPECT_THROW(store.create(ANA, std::string(61, 'x')), ValidationError);
    EXPECT_NO_THROW(store.create(ANA, std::string(60, 'x')));
}

TEST_F(PresetStoreTest, QuotesAreAddedInOrderAndCanBeRemoved)
{
    PresetStore store(*redis);
    const Preset preset = store.create(ANA, "Stoics");
    const Quote first = store.addQuote(ANA, preset.id, " You have power over your mind. ", "Marcus Aurelius");
    store.addQuote(ANA, preset.id, "We suffer more in imagination than in reality.", "Seneca");

    std::vector<Quote> quotes = store.find(ANA, preset.id)->quotes;
    ASSERT_EQ(quotes.size(), 2u);
    EXPECT_EQ(quotes[0].text, "You have power over your mind.");
    EXPECT_EQ(quotes[1].author, "Seneca");

    store.removeQuote(ANA, preset.id, first.id);
    quotes = store.find(ANA, preset.id)->quotes;
    ASSERT_EQ(quotes.size(), 1u);
    EXPECT_EQ(quotes[0].author, "Seneca");
    EXPECT_FALSE(redis->exists("quote:" + first.id));
}

TEST_F(PresetStoreTest, QuoteRules)
{
    PresetStore store(*redis);
    const Preset preset = store.create(ANA, "Stoics");
    EXPECT_THROW(store.addQuote(ANA, preset.id, "", "Seneca"), ValidationError);
    EXPECT_THROW(store.addQuote(ANA, preset.id, "Text", "  "), ValidationError);
    EXPECT_THROW(store.addQuote(ANA, preset.id, std::string(401, 'x'), "Seneca"), ValidationError);
    EXPECT_THROW(store.removeQuote(ANA, preset.id, "12345"), NotFoundError);
}

TEST_F(PresetStoreTest, RenameKeepsTheQuotes)
{
    PresetStore store(*redis);
    const Preset preset = store.create(ANA, "Stoics");
    store.addQuote(ANA, preset.id, "Text", "Seneca");

    const Preset renamed = store.rename(ANA, preset.id, "Stoic quotes");
    EXPECT_EQ(renamed.name, "Stoic quotes");
    EXPECT_EQ(renamed.quotes.size(), 1u);
}

TEST_F(PresetStoreTest, DeleteRemovesEveryKeyOfThePreset)
{
    PresetStore store(*redis);
    const Preset preset = store.create(ANA, "Stoics");
    store.addQuote(ANA, preset.id, "One", "A");
    store.addQuote(ANA, preset.id, "Two", "B");

    store.remove(ANA, preset.id);

    EXPECT_TRUE(store.list(ANA).empty());
    std::vector<std::string> keys;
    redis->keys("*", std::back_inserter(keys));
    for(const std::string& key : keys)
    {
        // Only the id counters may remain (Redis deletes a list once it is empty).
        EXPECT_TRUE(key.rfind("next:", 0) == 0) << "left behind: " << key;
    }
}

TEST_F(PresetStoreTest, AnotherUserCannotSeeOrChangeThePreset)
{
    PresetStore store(*redis);
    const Preset preset = store.create(ANA, "Stoics");
    const Quote quote = store.addQuote(ANA, preset.id, "Text", "Seneca");

    // To Bob, Ana's preset does not exist.
    EXPECT_TRUE(store.list(BOB).empty());
    EXPECT_FALSE(store.find(BOB, preset.id));
    EXPECT_THROW(store.rename(BOB, preset.id, "Mine now"), NotFoundError);
    EXPECT_THROW(store.addQuote(BOB, preset.id, "Text", "Bob"), NotFoundError);
    EXPECT_THROW(store.removeQuote(BOB, preset.id, quote.id), NotFoundError);
    EXPECT_THROW(store.remove(BOB, preset.id), NotFoundError);
    EXPECT_THROW(store.setActivePreset(BOB, preset.id), NotFoundError);

    // And nothing changed for Ana.
    EXPECT_EQ(store.find(ANA, preset.id)->name, "Stoics");
    EXPECT_EQ(store.find(ANA, preset.id)->quotes.size(), 1u);
}

TEST_F(PresetStoreTest, BadIdsAreNotFound)
{
    PresetStore store(*redis);
    EXPECT_FALSE(store.find(ANA, "abc"));
    EXPECT_FALSE(store.find(ANA, "../1"));
    EXPECT_THROW(store.remove(ANA, "999"), NotFoundError);
}

TEST_F(PresetStoreTest, ActivePresetDefaultsToStandard)
{
    PresetStore store(*redis);
    EXPECT_EQ(store.activePreset(ANA), STANDARD_PRESET_ID);

    const Preset preset = store.create(ANA, "Stoics");
    store.setActivePreset(ANA, preset.id);
    EXPECT_EQ(store.activePreset(ANA), preset.id);

    store.setActivePreset(ANA, STANDARD_PRESET_ID);
    EXPECT_EQ(store.activePreset(ANA), STANDARD_PRESET_ID);
}

TEST_F(PresetStoreTest, DeletingTheActivePresetGoesBackToStandard)
{
    PresetStore store(*redis);
    const Preset preset = store.create(ANA, "Stoics");
    store.setActivePreset(ANA, preset.id);

    store.remove(ANA, preset.id);
    EXPECT_EQ(store.activePreset(ANA), STANDARD_PRESET_ID);
}

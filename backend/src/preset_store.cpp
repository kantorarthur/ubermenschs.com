#include "preset_store.hpp"

#include "errors.hpp"
#include "redis_client.hpp"
#include "text.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    std::string userPresetsKey(const std::string& userId)
    {
        return "user:" + userId + ":presets";
    }

    std::string userSettingsKey(const std::string& userId)
    {
        return "user:" + userId + ":settings";
    }

    std::string presetKey(const std::string& presetId)
    {
        return "preset:" + presetId;
    }

    std::string presetQuotesKey(const std::string& presetId)
    {
        return "preset:" + presetId + ":quotes";
    }

    std::string quoteKey(const std::string& quoteId)
    {
        return "quote:" + quoteId;
    }

    std::string cleanPresetName(const std::string& name)
    {
        return requireText(name, limits::presetName, "The preset needs a name.",
                           "The name can have at most " + std::to_string(limits::presetName) + " characters.");
    }

    const std::string notFoundMessage = "This preset does not exist.";
}

PresetStore::PresetStore(sw::redis::Redis& redis)
    : redis_(redis)
{
}

bool PresetStore::owns(const std::string& userId, const std::string& presetId)
{
    if(!isId(presetId))
        return false;

    const auto owner = redis_.hget(presetKey(presetId), "userId");
    return owner && *owner == userId;
}

std::vector<std::string> PresetStore::quoteIds(const std::string& presetId)
{
    std::vector<std::string> ids;
    redis_.lrange(presetQuotesKey(presetId), 0, -1, std::back_inserter(ids));
    return ids;
}

std::vector<Quote> PresetStore::loadQuotes(const std::string& presetId)
{
    std::vector<Quote> quotes;
    for(const std::string& quoteId : quoteIds(presetId))
    {
        std::unordered_map<std::string, std::string> fields;
        redis_.hgetall(quoteKey(quoteId), std::inserter(fields, fields.begin()));
        if(fields.empty())
            continue;

        quotes.push_back(Quote{quoteId, fields["text"], fields["author"]});
    }
    return quotes;
}

std::vector<Preset> PresetStore::list(const std::string& userId)
{
    std::vector<std::string> ids;
    redis_.lrange(userPresetsKey(userId), 0, -1, std::back_inserter(ids));

    std::vector<Preset> presets;
    for(const std::string& presetId : ids)
    {
        std::optional<Preset> preset = find(userId, presetId);
        if(preset)
            presets.push_back(std::move(*preset));
    }
    return presets;
}

std::optional<Preset> PresetStore::find(const std::string& userId, const std::string& presetId)
{
    if(!owns(userId, presetId))
        return std::nullopt;

    // sw::redis::OptionalString has no value_or() in the C++11 build vcpkg makes, so we check by hand.
    const auto name = redis_.hget(presetKey(presetId), "name");
    return Preset{presetId, name ? *name : std::string(), loadQuotes(presetId)};
}

Preset PresetStore::create(const std::string& userId, const std::string& name)
{
    const std::string cleanedName = cleanPresetName(name);
    const std::string presetId = std::to_string(redis_.incr("next:preset:id"));

    // All three writes happen together or not at all.
    auto transaction = redis_.transaction();
    transaction.hset(presetKey(presetId), "name", cleanedName)
               .hset(presetKey(presetId), "userId", userId)
               .rpush(userPresetsKey(userId), presetId);
    transaction.exec();
    waitForReplica(transaction.redis());

    return Preset{presetId, cleanedName, {}};
}

Preset PresetStore::rename(const std::string& userId, const std::string& presetId, const std::string& name)
{
    const std::string cleanedName = cleanPresetName(name);
    if(!owns(userId, presetId))
        throw NotFoundError(notFoundMessage);

    redis_.hset(presetKey(presetId), "name", cleanedName);
    return Preset{presetId, cleanedName, loadQuotes(presetId)};
}

void PresetStore::remove(const std::string& userId, const std::string& presetId)
{
    if(!owns(userId, presetId))
        throw NotFoundError(notFoundMessage);

    const std::vector<std::string> quotes = quoteIds(presetId);
    const bool wasActive = activePreset(userId) == presetId;

    // Redis has no "delete with everything that belongs to it", so we delete each part,
    // all in one transaction.
    auto transaction = redis_.transaction();
    transaction.del(presetKey(presetId))
               .del(presetQuotesKey(presetId))
               .lrem(userPresetsKey(userId), 0, presetId);
    for(const std::string& quoteId : quotes)
        transaction.del(quoteKey(quoteId));
    if(wasActive)
        transaction.hset(userSettingsKey(userId), "activePreset", STANDARD_PRESET_ID);
    transaction.exec();
}

Quote PresetStore::addQuote(const std::string& userId, const std::string& presetId,
                            const std::string& text, const std::string& author)
{
    const std::string cleanedText = requireText(text, limits::quoteText, "The quote cannot be empty.",
        "The quote can have at most " + std::to_string(limits::quoteText) + " characters.");
    const std::string cleanedAuthor = requireText(author, limits::quoteAuthor, "Add the author of the quote.",
        "The author can have at most " + std::to_string(limits::quoteAuthor) + " characters.");

    if(!owns(userId, presetId))
        throw NotFoundError(notFoundMessage);

    const std::string quoteId = std::to_string(redis_.incr("next:quote:id"));

    auto transaction = redis_.transaction();
    transaction.hset(quoteKey(quoteId), "text", cleanedText)
               .hset(quoteKey(quoteId), "author", cleanedAuthor)
               .rpush(presetQuotesKey(presetId), quoteId);
    transaction.exec();
    waitForReplica(transaction.redis());

    return Quote{quoteId, cleanedText, cleanedAuthor};
}

void PresetStore::removeQuote(const std::string& userId, const std::string& presetId, const std::string& quoteId)
{
    if(!owns(userId, presetId))
        throw NotFoundError(notFoundMessage);

    // Only a quote of this preset may be deleted.
    const std::vector<std::string> ids = quoteIds(presetId);
    if(std::find(ids.begin(), ids.end(), quoteId) == ids.end())
        throw NotFoundError("This quote does not exist.");

    auto transaction = redis_.transaction();
    transaction.lrem(presetQuotesKey(presetId), 0, quoteId)
               .del(quoteKey(quoteId));
    transaction.exec();
}

std::string PresetStore::activePreset(const std::string& userId)
{
    const auto active = redis_.hget(userSettingsKey(userId), "activePreset");
    if(!active || *active == STANDARD_PRESET_ID)
        return STANDARD_PRESET_ID;

    // A preset that no longer exists falls back to Standard.
    if(!owns(userId, *active))
        return STANDARD_PRESET_ID;
    return *active;
}

void PresetStore::setActivePreset(const std::string& userId, const std::string& presetId)
{
    if(presetId != STANDARD_PRESET_ID && !owns(userId, presetId))
        throw NotFoundError(notFoundMessage);

    redis_.hset(userSettingsKey(userId), "activePreset", presetId);
}

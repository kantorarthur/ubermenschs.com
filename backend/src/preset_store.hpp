#pragma once

#include <sw/redis++/redis++.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

struct Quote
{
    std::string id;
    std::string text;
    std::string author;
};

struct Preset
{
    std::string id;
    std::string name;
    std::vector<Quote> quotes;
};

// The fixed preset that lives in the frontend; "activePreset" holds this when no own preset is chosen.
inline const std::string STANDARD_PRESET_ID = "standard";

// The same limits the frontend shows (src/api/presets.ts, LIMITS).
namespace limits
{
    inline constexpr std::size_t presetName = 60;
    inline constexpr std::size_t quoteText = 400;
    inline constexpr std::size_t quoteAuthor = 100;
}

// Reads and writes quote presets in Redis, using the keys from the plan:
//
//   next:preset:id, next:quote:id   counters for new ids (INCR)
//   user:{id}:presets               list of the user's preset ids
//   user:{id}:settings              hash with "activePreset"
//   preset:{id}                     hash: name, userId
//   preset:{id}:quotes              list of the preset's quote ids
//   quote:{id}                      hash: text, author
//
// Every method checks that the preset belongs to the user; a preset of someone else
// is reported as "does not exist" (NotFoundError), so ids cannot be probed.
class PresetStore
{
public:
    explicit PresetStore(sw::redis::Redis& redis);

    std::vector<Preset> list(const std::string& userId);
    std::optional<Preset> find(const std::string& userId, const std::string& presetId);

    Preset create(const std::string& userId, const std::string& name);
    Preset rename(const std::string& userId, const std::string& presetId, const std::string& name);
    void remove(const std::string& userId, const std::string& presetId);

    Quote addQuote(const std::string& userId, const std::string& presetId,
                   const std::string& text, const std::string& author);
    void removeQuote(const std::string& userId, const std::string& presetId, const std::string& quoteId);

    // "standard" or the id of one of the user's presets.
    std::string activePreset(const std::string& userId);
    void setActivePreset(const std::string& userId, const std::string& presetId);

private:
    bool owns(const std::string& userId, const std::string& presetId);
    std::vector<std::string> quoteIds(const std::string& presetId);
    std::vector<Quote> loadQuotes(const std::string& presetId);

    sw::redis::Redis& redis_;
};

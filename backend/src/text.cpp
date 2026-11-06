#include "text.hpp"

#include "errors.hpp"

#include <algorithm>
#include <cctype>

std::string trim(const std::string& text)
{
    const auto isSpace = [](unsigned char c)
    {
        return std::isspace(c) != 0;
    };

    const auto first = std::find_if_not(text.begin(), text.end(), isSpace);
    const auto last = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
    if(first >= last)
        return "";
    return std::string(first, last);
}

std::size_t utf8Length(const std::string& text)
{
    // In UTF-8 every character has exactly one byte that does not start with the bits 10.
    std::size_t count = 0;
    for(const unsigned char c : text)
    {
        if((c & 0xC0) != 0x80)
            ++count;
    }
    return count;
}

std::string normalizeEmail(const std::string& email)
{
    std::string normalized = trim(email);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    return normalized;
}

bool isId(const std::string& text)
{
    if(text.empty() || text.size() > 18)
        return false;

    return std::all_of(text.begin(), text.end(), [](unsigned char c)
    {
        return c >= '0' && c <= '9';
    });
}

std::string requireText(const std::string& text, std::size_t maxCharacters,
                        const std::string& emptyMessage, const std::string& tooLongMessage)
{
    std::string cleaned = trim(text);
    if(cleaned.empty())
        throw ValidationError(emptyMessage);
    if(utf8Length(cleaned) > maxCharacters)
        throw ValidationError(tooLongMessage);
    return cleaned;
}

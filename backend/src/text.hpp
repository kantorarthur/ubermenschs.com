#pragma once

#include <cstddef>
#include <string>

// Removes spaces, tabs and line breaks from both ends.
std::string trim(const std::string& text);

// Number of characters (not bytes) in UTF-8 text: "Übermensch" is 10, although it takes 11 bytes.
std::size_t utf8Length(const std::string& text);

// Trims an email and lowers its letters, so "Ana@Mail.com " and "ana@mail.com" are one account.
std::string normalizeEmail(const std::string& email);

// True for a non-empty string made only of the digits 0-9 (our ids).
bool isId(const std::string& text);

// Trims the text, then throws ValidationError with the given messages
// if it is empty or longer than maxCharacters.
std::string requireText(const std::string& text, std::size_t maxCharacters,
                        const std::string& emptyMessage, const std::string& tooLongMessage);

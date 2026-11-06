#include "crypto.hpp"

#include <sodium.h>

#include <array>
#include <stdexcept>
#include <string>

namespace
{
    std::string toHex(const unsigned char* bytes, std::size_t length)
    {
        std::string hex(length * 2 + 1, '\0');
        sodium_bin2hex(hex.data(), hex.size(), bytes, length);
        hex.pop_back(); // sodium_bin2hex writes a closing '\0'
        return hex;
    }
}

bool initCrypto()
{
    // sodium_init returns 0 on success and 1 if it was already initialised.
    return sodium_init() >= 0;
}

std::string hashPassword(const std::string& password)
{
    std::array<char, crypto_pwhash_STRBYTES> hash{};

    // The "interactive" settings: about 64 MB of memory and a fraction of a second per hash,
    // which is slow enough to make guessing passwords expensive.
    const int result = crypto_pwhash_str(hash.data(), password.data(), password.size(),
                                         crypto_pwhash_OPSLIMIT_INTERACTIVE,
                                         crypto_pwhash_MEMLIMIT_INTERACTIVE);
    if(result != 0)
        throw std::runtime_error("Password hashing failed (out of memory)");
    return std::string(hash.data());
}

bool verifyPassword(const std::string& hash, const std::string& password)
{
    return crypto_pwhash_str_verify(hash.c_str(), password.data(), password.size()) == 0;
}

std::string randomToken()
{
    std::array<unsigned char, 32> bytes{};
    randombytes_buf(bytes.data(), bytes.size());
    return toHex(bytes.data(), bytes.size());
}

std::string fingerprint(const std::string& token)
{
    std::array<unsigned char, crypto_generichash_BYTES> digest{};
    crypto_generichash(digest.data(), digest.size(),
                       reinterpret_cast<const unsigned char*>(token.data()), token.size(),
                       nullptr, 0);
    return toHex(digest.data(), digest.size());
}

#pragma once

#include <string>

// Password hashing and random tokens, all done by libsodium.

// Must be called once at start-up, before any other function here. Returns false on failure.
bool initCrypto();

// Argon2id hash of the password, as one string that also holds the salt and the settings,
// e.g. "$argon2id$v=19$m=65536,t=2,p=1$...". Only this is stored, never the password.
std::string hashPassword(const std::string& password);

// True if the password matches a hash made by hashPassword.
bool verifyPassword(const std::string& hash, const std::string& password);

// 32 random bytes as 64 hex characters; used as the session token in the cookie.
std::string randomToken();

// A one-way fingerprint (BLAKE2b) of a token, as hex. Redis stores this instead of the token,
// so someone who reads the database still cannot use the sessions in it.
std::string fingerprint(const std::string& token);

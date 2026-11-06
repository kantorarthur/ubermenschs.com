#pragma once

#include <string>

// The cryptography of Web Push, done with OpenSSL:
//
//   - VAPID (RFC 8292): the server proves who it is to the push service with a JWT signed by its
//     P-256 key (ES256). The browser got the public half when it subscribed.
//   - Message encryption (RFC 8291, "aes128gcm"): only the browser that subscribed can read the
//     notification; the push service in between (Google, Mozilla, Apple, ...) cannot.
//
// Keys and binary values are std::string holding raw bytes; base64url is used only at the edges
// (JSON, HTTP headers).

// base64url without padding ("-" and "_" instead of "+" and "/"), as Web Push uses everywhere.
std::string base64UrlEncode(const std::string& bytes);

// Accepts base64url with or without "=" padding. Throws std::invalid_argument on bad input.
std::string base64UrlDecode(const std::string& text);

// A P-256 key pair: a 32-byte private key and a 65-byte uncompressed public key (0x04 || x || y).
struct P256KeyPair
{
    std::string privateKey;
    std::string publicKey;
};

P256KeyPair generateP256KeyPair();

// The public key that belongs to a 32-byte private key. Throws std::invalid_argument if it is not one.
std::string p256PublicKeyOf(const std::string& privateKey);

// True for a 65-byte uncompressed point that lies on the P-256 curve.
bool isP256PublicKey(const std::string& publicKey);

// Encrypts a notification for one browser (RFC 8291). receiverPublicKey and authSecret come from the
// browser's subscription ("p256dh", 65 bytes, and "auth", 16 bytes). Every call uses a new random
// salt and a new random sender key, so encrypting the same text twice gives different bytes.
std::string encryptPushPayload(const std::string& plaintext, const std::string& receiverPublicKey,
                               const std::string& authSecret);

// The same with a fixed sender key and salt, so the result can be compared with the example in RFC 8291.
// Only for tests: never reuse a sender key or salt for real messages.
std::string encryptPushPayloadWith(const std::string& plaintext, const std::string& receiverPublicKey,
                                   const std::string& authSecret, const std::string& senderPrivateKey,
                                   const std::string& salt);

// A VAPID JWT (RFC 8292): {"aud": audience, "exp": expiresAt, "sub": subject}, signed with ES256.
// audience is the push service's origin, e.g. "https://fcm.googleapis.com";
// expiresAt is in seconds since 1970 and must be at most 24 hours ahead.
std::string vapidJwt(const std::string& audience, long long expiresAt, const std::string& subject,
                     const std::string& privateKey);

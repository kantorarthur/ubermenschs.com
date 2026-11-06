#include "webpush_crypto.hpp"

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/param_build.h>
#include <openssl/rand.h>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
    // OpenSSL objects are freed automatically when these go out of scope.
    struct Free
    {
        void operator()(EVP_PKEY* p) const
        {
            EVP_PKEY_free(p);
        }
        void operator()(EVP_PKEY_CTX* p) const
        {
            EVP_PKEY_CTX_free(p);
        }
        void operator()(EVP_CIPHER_CTX* p) const
        {
            EVP_CIPHER_CTX_free(p);
        }
        void operator()(EVP_MD_CTX* p) const
        {
            EVP_MD_CTX_free(p);
        }
        void operator()(BIGNUM* p) const
        {
            BN_clear_free(p);
        }
        void operator()(EC_GROUP* p) const
        {
            EC_GROUP_free(p);
        }
        void operator()(EC_POINT* p) const
        {
            EC_POINT_free(p);
        }
        void operator()(ECDSA_SIG* p) const
        {
            ECDSA_SIG_free(p);
        }
        void operator()(OSSL_PARAM_BLD* p) const
        {
            OSSL_PARAM_BLD_free(p);
        }
        void operator()(OSSL_PARAM* p) const
        {
            OSSL_PARAM_free(p);
        }
    };

    template <typename T>
    using Owned = std::unique_ptr<T, Free>;

    constexpr std::size_t PRIVATE_KEY_SIZE = 32;
    constexpr std::size_t PUBLIC_KEY_SIZE = 65;
    constexpr std::size_t AUTH_SECRET_SIZE = 16;
    constexpr std::size_t SALT_SIZE = 16;
    constexpr std::uint32_t RECORD_SIZE = 4096;

    void check(bool ok, const char* what)
    {
        if(!ok)
            throw std::runtime_error(std::string("OpenSSL failed: ") + what);
    }

    const unsigned char* bytes(const std::string& text)
    {
        return reinterpret_cast<const unsigned char*>(text.data());
    }

    std::string randomBytes(std::size_t count)
    {
        std::string out(count, '\0');
        check(RAND_bytes(reinterpret_cast<unsigned char*>(out.data()), static_cast<int>(count)) == 1, "RAND_bytes");
        return out;
    }

    std::string hmacSha256(const std::string& key, const std::string& data)
    {
        std::array<unsigned char, 32> out{};
        unsigned int length = 0;
        check(HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()), bytes(data), data.size(),
                   out.data(), &length) != nullptr, "HMAC");
        return std::string(reinterpret_cast<const char*>(out.data()), length);
    }

    // The public point d·G of a private key d, uncompressed.
    std::string publicPointOf(const BIGNUM* privateKey)
    {
        Owned<EC_GROUP> group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
        check(group != nullptr, "EC_GROUP_new_by_curve_name");
        Owned<EC_POINT> point(EC_POINT_new(group.get()));
        check(point != nullptr, "EC_POINT_new");
        check(EC_POINT_mul(group.get(), point.get(), privateKey, nullptr, nullptr, nullptr) == 1, "EC_POINT_mul");

        std::string out(PUBLIC_KEY_SIZE, '\0');
        const std::size_t written = EC_POINT_point2oct(group.get(), point.get(), POINT_CONVERSION_UNCOMPRESSED,
                                                       reinterpret_cast<unsigned char*>(out.data()), out.size(),
                                                       nullptr);
        check(written == PUBLIC_KEY_SIZE, "EC_POINT_point2oct");
        return out;
    }

    // Builds an EVP_PKEY from raw parts; privateKey may be null for a public-only key.
    Owned<EVP_PKEY> makeKey(const BIGNUM* privateKey, const std::string& publicKey)
    {
        Owned<OSSL_PARAM_BLD> builder(OSSL_PARAM_BLD_new());
        check(builder != nullptr, "OSSL_PARAM_BLD_new");
        check(OSSL_PARAM_BLD_push_utf8_string(builder.get(), OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0) == 1,
              "group name");
        check(OSSL_PARAM_BLD_push_octet_string(builder.get(), OSSL_PKEY_PARAM_PUB_KEY, publicKey.data(),
                                               publicKey.size()) == 1, "public key");
        if(privateKey != nullptr)
            check(OSSL_PARAM_BLD_push_BN(builder.get(), OSSL_PKEY_PARAM_PRIV_KEY, privateKey) == 1, "private key");

        Owned<OSSL_PARAM> params(OSSL_PARAM_BLD_to_param(builder.get()));
        check(params != nullptr, "OSSL_PARAM_BLD_to_param");

        Owned<EVP_PKEY_CTX> context(EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr));
        check(context != nullptr, "EVP_PKEY_CTX_new_from_name");
        check(EVP_PKEY_fromdata_init(context.get()) == 1, "EVP_PKEY_fromdata_init");

        EVP_PKEY* key = nullptr;
        const int selection = privateKey != nullptr ? EVP_PKEY_KEYPAIR : EVP_PKEY_PUBLIC_KEY;
        if(EVP_PKEY_fromdata(context.get(), &key, selection, params.get()) != 1)
            throw std::invalid_argument("not a P-256 key");
        return Owned<EVP_PKEY>(key);
    }

    Owned<BIGNUM> privateScalar(const std::string& privateKey)
    {
        if(privateKey.size() != PRIVATE_KEY_SIZE)
            throw std::invalid_argument("a P-256 private key has 32 bytes");
        Owned<BIGNUM> scalar(BN_bin2bn(bytes(privateKey), static_cast<int>(privateKey.size()), nullptr));
        check(scalar != nullptr, "BN_bin2bn");
        if(BN_is_zero(scalar.get()))
            throw std::invalid_argument("the private key cannot be zero");
        return scalar;
    }

    // The public key of a private key, after checking that it is a valid P-256 private key.
    std::string publicKeyOfPrivate(const std::string& privateKey)
    {
        const Owned<BIGNUM> scalar = privateScalar(privateKey);
        // The scalar must be smaller than the order of the curve.
        Owned<EC_GROUP> group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
        check(group != nullptr, "EC_GROUP_new_by_curve_name");
        if(BN_cmp(scalar.get(), EC_GROUP_get0_order(group.get())) >= 0)
            throw std::invalid_argument("not a P-256 private key");
        return publicPointOf(scalar.get());
    }

    Owned<EVP_PKEY> privateKeyFrom(const std::string& privateKey)
    {
        const std::string publicKey = publicKeyOfPrivate(privateKey);
        const Owned<BIGNUM> scalar = privateScalar(privateKey);
        return makeKey(scalar.get(), publicKey);
    }

    Owned<EVP_PKEY> publicKeyFrom(const std::string& publicKey)
    {
        if(!isP256PublicKey(publicKey))
            throw std::invalid_argument("not a P-256 public key");
        return makeKey(nullptr, publicKey);
    }

    // Elliptic-curve Diffie-Hellman: the 32-byte secret both sides can compute.
    std::string ecdh(EVP_PKEY* ownKey, EVP_PKEY* peerKey)
    {
        Owned<EVP_PKEY_CTX> context(EVP_PKEY_CTX_new(ownKey, nullptr));
        check(context != nullptr, "EVP_PKEY_CTX_new");
        check(EVP_PKEY_derive_init(context.get()) == 1, "EVP_PKEY_derive_init");
        check(EVP_PKEY_derive_set_peer(context.get(), peerKey) == 1, "EVP_PKEY_derive_set_peer");

        std::size_t length = 0;
        check(EVP_PKEY_derive(context.get(), nullptr, &length) == 1, "EVP_PKEY_derive (size)");
        std::string secret(length, '\0');
        check(EVP_PKEY_derive(context.get(), reinterpret_cast<unsigned char*>(secret.data()), &length) == 1,
              "EVP_PKEY_derive");
        secret.resize(length);
        return secret;
    }

    // AES-128-GCM: the ciphertext followed by the 16-byte tag.
    std::string aes128Gcm(const std::string& key, const std::string& nonce, const std::string& plaintext)
    {
        Owned<EVP_CIPHER_CTX> context(EVP_CIPHER_CTX_new());
        check(context != nullptr, "EVP_CIPHER_CTX_new");
        check(EVP_EncryptInit_ex(context.get(), EVP_aes_128_gcm(), nullptr, bytes(key), bytes(nonce)) == 1,
              "EVP_EncryptInit_ex");

        std::string out(plaintext.size() + 16, '\0');
        auto* cursor = reinterpret_cast<unsigned char*>(out.data());
        int length = 0;
        check(EVP_EncryptUpdate(context.get(), cursor, &length, bytes(plaintext), static_cast<int>(plaintext.size()))
              == 1, "EVP_EncryptUpdate");
        int finalLength = 0;
        check(EVP_EncryptFinal_ex(context.get(), cursor + length, &finalLength) == 1, "EVP_EncryptFinal_ex");
        check(EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, 16, cursor + length + finalLength) == 1,
              "GCM tag");
        out.resize(static_cast<std::size_t>(length + finalLength) + 16);
        return out;
    }

    std::string jsonEscape(const std::string& text)
    {
        std::string out;
        for(const char c : text)
        {
            if(c == '"' || c == '\\')
                out += '\\';
            if(static_cast<unsigned char>(c) >= 0x20)
                out += c;
        }
        return out;
    }

    const char* const BASE64_URL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
}

std::string base64UrlEncode(const std::string& input)
{
    std::string out;
    std::size_t i = 0;
    for(; i + 2 < input.size(); i += 3)
    {
        const std::uint32_t n = (static_cast<std::uint8_t>(input[i]) << 16)
                              | (static_cast<std::uint8_t>(input[i + 1]) << 8)
                              | static_cast<std::uint8_t>(input[i + 2]);
        out += BASE64_URL[(n >> 18) & 63];
        out += BASE64_URL[(n >> 12) & 63];
        out += BASE64_URL[(n >> 6) & 63];
        out += BASE64_URL[n & 63];
    }

    const std::size_t rest = input.size() - i;
    if(rest == 1)
    {
        const std::uint32_t n = static_cast<std::uint8_t>(input[i]) << 16;
        out += BASE64_URL[(n >> 18) & 63];
        out += BASE64_URL[(n >> 12) & 63];
    }
    else if(rest == 2)
    {
        const std::uint32_t n = (static_cast<std::uint8_t>(input[i]) << 16)
                              | (static_cast<std::uint8_t>(input[i + 1]) << 8);
        out += BASE64_URL[(n >> 18) & 63];
        out += BASE64_URL[(n >> 12) & 63];
        out += BASE64_URL[(n >> 6) & 63];
    }
    return out;
}

std::string base64UrlDecode(const std::string& text)
{
    std::string trimmed = text;
    while(!trimmed.empty() && trimmed.back() == '=')
        trimmed.pop_back();
    if(trimmed.size() % 4 == 1)
        throw std::invalid_argument("not base64url");

    std::string out;
    std::uint32_t buffer = 0;
    int bits = 0;
    for(const char c : trimmed)
    {
        int value = -1;
        if(c >= 'A' && c <= 'Z')
            value = c - 'A';
        else if(c >= 'a' && c <= 'z')
            value = c - 'a' + 26;
        else if(c >= '0' && c <= '9')
            value = c - '0' + 52;
        else if(c == '-' || c == '+')
            value = 62;
        else if(c == '_' || c == '/')
            value = 63;
        if(value < 0)
            throw std::invalid_argument("not base64url");

        buffer = (buffer << 6) | static_cast<std::uint32_t>(value);
        bits += 6;
        if(bits >= 8)
        {
            bits -= 8;
            out += static_cast<char>((buffer >> bits) & 0xFF);
        }
    }
    return out;
}

P256KeyPair generateP256KeyPair()
{
    Owned<EVP_PKEY> key(EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256"));
    check(key != nullptr, "EVP_PKEY_Q_keygen");

    BIGNUM* scalar = nullptr;
    check(EVP_PKEY_get_bn_param(key.get(), OSSL_PKEY_PARAM_PRIV_KEY, &scalar) == 1, "private key");
    const Owned<BIGNUM> owned(scalar);

    std::string privateKey(PRIVATE_KEY_SIZE, '\0');
    check(BN_bn2binpad(owned.get(), reinterpret_cast<unsigned char*>(privateKey.data()), PRIVATE_KEY_SIZE)
          == static_cast<int>(PRIVATE_KEY_SIZE), "BN_bn2binpad");
    return P256KeyPair{privateKey, publicPointOf(owned.get())};
}

std::string p256PublicKeyOf(const std::string& privateKey)
{
    return publicKeyOfPrivate(privateKey);
}

bool isP256PublicKey(const std::string& publicKey)
{
    if(publicKey.size() != PUBLIC_KEY_SIZE || publicKey[0] != 0x04)
        return false;

    Owned<EC_GROUP> group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    Owned<EC_POINT> point(EC_POINT_new(group.get()));
    if(group == nullptr || point == nullptr)
        return false;
    // point2oct's opposite also checks that the point is on the curve.
    return EC_POINT_oct2point(group.get(), point.get(), bytes(publicKey), publicKey.size(), nullptr) == 1
        && EC_POINT_is_on_curve(group.get(), point.get(), nullptr) == 1;
}

std::string encryptPushPayload(const std::string& plaintext, const std::string& receiverPublicKey,
                               const std::string& authSecret)
{
    const P256KeyPair sender = generateP256KeyPair();
    return encryptPushPayloadWith(plaintext, receiverPublicKey, authSecret, sender.privateKey,
                                  randomBytes(SALT_SIZE));
}

std::string encryptPushPayloadWith(const std::string& plaintext, const std::string& receiverPublicKey,
                                   const std::string& authSecret, const std::string& senderPrivateKey,
                                   const std::string& salt)
{
    if(authSecret.size() != AUTH_SECRET_SIZE)
        throw std::invalid_argument("the auth secret has 16 bytes");
    if(salt.size() != SALT_SIZE)
        throw std::invalid_argument("the salt has 16 bytes");
    // One record holds the text, a delimiter byte and the 16-byte tag.
    if(plaintext.size() + 1 + 16 > RECORD_SIZE)
        throw std::invalid_argument("the message is too long");

    const Owned<EVP_PKEY> senderKey = privateKeyFrom(senderPrivateKey);
    const Owned<EVP_PKEY> receiverKey = publicKeyFrom(receiverPublicKey);
    const std::string senderPublicKey = p256PublicKeyOf(senderPrivateKey);

    // RFC 8291, section 3.4: combine the shared secret with the auth secret...
    const std::string sharedSecret = ecdh(senderKey.get(), receiverKey.get());
    const std::string prkKey = hmacSha256(authSecret, sharedSecret);
    const std::string keyInfo = std::string("WebPush: info") + '\0' + receiverPublicKey + senderPublicKey;
    const std::string ikm = hmacSha256(prkKey, keyInfo + '\x01');

    // ...then RFC 8188: the content key and the nonce come from the salt.
    const std::string prk = hmacSha256(salt, ikm);
    const std::string contentKey = hmacSha256(prk, std::string("Content-Encoding: aes128gcm") + '\0' + '\x01')
                                       .substr(0, 16);
    const std::string nonce = hmacSha256(prk, std::string("Content-Encoding: nonce") + '\0' + '\x01')
                                  .substr(0, 12);

    // The only (so the last) record ends with the delimiter 0x02.
    const std::string ciphertext = aes128Gcm(contentKey, nonce, plaintext + '\x02');

    // Header: salt (16) | record size (4, big endian) | key id length (1) | key id = sender public key (65).
    std::string header = salt;
    header += static_cast<char>((RECORD_SIZE >> 24) & 0xFF);
    header += static_cast<char>((RECORD_SIZE >> 16) & 0xFF);
    header += static_cast<char>((RECORD_SIZE >> 8) & 0xFF);
    header += static_cast<char>(RECORD_SIZE & 0xFF);
    header += static_cast<char>(PUBLIC_KEY_SIZE);
    header += senderPublicKey;
    return header + ciphertext;
}

std::string vapidJwt(const std::string& audience, long long expiresAt, const std::string& subject,
                     const std::string& privateKey)
{
    const std::string header = base64UrlEncode(R"({"typ":"JWT","alg":"ES256"})");
    const std::string claims = base64UrlEncode("{\"aud\":\"" + jsonEscape(audience) + "\",\"exp\":"
                                               + std::to_string(expiresAt) + ",\"sub\":\""
                                               + jsonEscape(subject) + "\"}");
    const std::string signingInput = header + "." + claims;

    const Owned<EVP_PKEY> key = privateKeyFrom(privateKey);
    Owned<EVP_MD_CTX> context(EVP_MD_CTX_new());
    check(context != nullptr, "EVP_MD_CTX_new");
    check(EVP_DigestSignInit(context.get(), nullptr, EVP_sha256(), nullptr, key.get()) == 1, "EVP_DigestSignInit");

    std::size_t length = 0;
    check(EVP_DigestSign(context.get(), nullptr, &length, bytes(signingInput), signingInput.size()) == 1,
          "EVP_DigestSign (size)");
    std::string der(length, '\0');
    check(EVP_DigestSign(context.get(), reinterpret_cast<unsigned char*>(der.data()), &length,
                         bytes(signingInput), signingInput.size()) == 1, "EVP_DigestSign");

    // OpenSSL gives the signature in DER; a JWT wants r and s as two 32-byte numbers.
    const unsigned char* cursor = bytes(der);
    Owned<ECDSA_SIG> signature(d2i_ECDSA_SIG(nullptr, &cursor, static_cast<long>(length)));
    check(signature != nullptr, "d2i_ECDSA_SIG");
    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(signature.get(), &r, &s);

    std::string raw(64, '\0');
    check(BN_bn2binpad(r, reinterpret_cast<unsigned char*>(raw.data()), 32) == 32, "r");
    check(BN_bn2binpad(s, reinterpret_cast<unsigned char*>(raw.data()) + 32, 32) == 32, "s");
    return signingInput + "." + base64UrlEncode(raw);
}

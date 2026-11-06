#include "webpush_crypto.hpp"

#include <gtest/gtest.h>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>

#include <stdexcept>
#include <string>

namespace
{
    // The example of RFC 8291 (section 5 and appendix A).
    const std::string RFC_PLAINTEXT = "When I grow up, I want to be a watermelon";
    const std::string RFC_SENDER_PRIVATE = "yfWPiYE-n46HLnH0KqZOF1fJJU3MYrct3AELtAQ-oRw";
    const std::string RFC_SENDER_PUBLIC =
        "BP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A8";
    const std::string RFC_RECEIVER_PUBLIC =
        "BCVxsr7N_eNgVRqvHtD0zTZsEc6-VV-JvLexhqUzORcxaOzi6-AYWXvTBHm4bjyPjs7Vd8pZGH6SRpkNtoIAiw4";
    const std::string RFC_SALT = "DGv6ra1nlYgDCS1FRnbzlw";
    const std::string RFC_AUTH_SECRET = "BTBZMqHH6r4Tts7J_aSIgg";
    const std::string RFC_MESSAGE =
        "DGv6ra1nlYgDCS1FRnbzlwAAEABBBP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A_"
        "yl95bQpu6cVPTpK4Mqgkf1CXztLVBSt2Ks3oZwbuwXPXLWyouBWLVWGNWQexSgSxsj_Qulcy4a-fN";

    // Checks an ES256 signature (r || s, 64 bytes) with OpenSSL, independently of vapidJwt.
    bool verifyEs256(const std::string& signingInput, const std::string& rawSignature, const std::string& publicKey)
    {
        OSSL_PARAM_BLD* builder = OSSL_PARAM_BLD_new();
        OSSL_PARAM_BLD_push_utf8_string(builder, OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0);
        OSSL_PARAM_BLD_push_octet_string(builder, OSSL_PKEY_PARAM_PUB_KEY, publicKey.data(), publicKey.size());
        OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(builder);
        EVP_PKEY_CTX* keyContext = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
        EVP_PKEY* key = nullptr;
        EVP_PKEY_fromdata_init(keyContext);
        EVP_PKEY_fromdata(keyContext, &key, EVP_PKEY_PUBLIC_KEY, params);

        ECDSA_SIG* signature = ECDSA_SIG_new();
        const auto* raw = reinterpret_cast<const unsigned char*>(rawSignature.data());
        ECDSA_SIG_set0(signature, BN_bin2bn(raw, 32, nullptr), BN_bin2bn(raw + 32, 32, nullptr));
        unsigned char* der = nullptr;
        const int derLength = i2d_ECDSA_SIG(signature, &der);

        EVP_MD_CTX* context = EVP_MD_CTX_new();
        EVP_DigestVerifyInit(context, nullptr, EVP_sha256(), nullptr, key);
        const bool ok = EVP_DigestVerify(context, der, static_cast<std::size_t>(derLength),
                                         reinterpret_cast<const unsigned char*>(signingInput.data()),
                                         signingInput.size()) == 1;

        EVP_MD_CTX_free(context);
        OPENSSL_free(der);
        ECDSA_SIG_free(signature);
        EVP_PKEY_free(key);
        EVP_PKEY_CTX_free(keyContext);
        OSSL_PARAM_free(params);
        OSSL_PARAM_BLD_free(builder);
        return ok;
    }
}

TEST(Base64Url, EncodesWithoutPadding)
{
    EXPECT_EQ(base64UrlEncode(""), "");
    EXPECT_EQ(base64UrlEncode("f"), "Zg");
    EXPECT_EQ(base64UrlEncode("fo"), "Zm8");
    EXPECT_EQ(base64UrlEncode("foo"), "Zm9v");
    EXPECT_EQ(base64UrlEncode("foob"), "Zm9vYg");
    EXPECT_EQ(base64UrlEncode(std::string("\xfb\xff", 2)), "-_8");
}

TEST(Base64Url, DecodesWithOrWithoutPadding)
{
    EXPECT_EQ(base64UrlDecode("Zm9vYg"), "foob");
    EXPECT_EQ(base64UrlDecode("Zm9vYg=="), "foob");
    EXPECT_EQ(base64UrlDecode("-_8"), std::string("\xfb\xff", 2));
    EXPECT_THROW(base64UrlDecode("Zm9v!"), std::invalid_argument);
    EXPECT_THROW(base64UrlDecode("Z"), std::invalid_argument);

    const std::string all = std::string("\0\x01\x7f\x80\xfe\xff", 6) + "Übermensch";
    EXPECT_EQ(base64UrlDecode(base64UrlEncode(all)), all);
}

TEST(WebPushCrypto, MatchesTheExampleOfRfc8291)
{
    EXPECT_EQ(base64UrlEncode(p256PublicKeyOf(base64UrlDecode(RFC_SENDER_PRIVATE))), RFC_SENDER_PUBLIC);

    const std::string message = encryptPushPayloadWith(
        RFC_PLAINTEXT, base64UrlDecode(RFC_RECEIVER_PUBLIC), base64UrlDecode(RFC_AUTH_SECRET),
        base64UrlDecode(RFC_SENDER_PRIVATE), base64UrlDecode(RFC_SALT));
    EXPECT_EQ(base64UrlEncode(message), RFC_MESSAGE);
}

TEST(WebPushCrypto, EveryMessageIsEncryptedDifferently)
{
    const P256KeyPair receiver = generateP256KeyPair();
    const std::string auth = base64UrlDecode(RFC_AUTH_SECRET);

    const std::string first = encryptPushPayload("Read 20 pages.", receiver.publicKey, auth);
    const std::string second = encryptPushPayload("Read 20 pages.", receiver.publicKey, auth);
    EXPECT_NE(first, second);
    // header (86) + text + delimiter (1) + tag (16)
    EXPECT_EQ(first.size(), 86u + 14u + 1u + 16u);
}

TEST(WebPushCrypto, RefusesBadInput)
{
    const P256KeyPair receiver = generateP256KeyPair();
    const std::string auth = base64UrlDecode(RFC_AUTH_SECRET);

    EXPECT_THROW(encryptPushPayload("Hi", receiver.publicKey, "short"), std::invalid_argument);
    EXPECT_THROW(encryptPushPayload("Hi", receiver.publicKey.substr(1), auth), std::invalid_argument);
    EXPECT_THROW(encryptPushPayload(std::string(4080, 'x'), receiver.publicKey, auth), std::invalid_argument);
    EXPECT_THROW(p256PublicKeyOf(std::string(32, '\0')), std::invalid_argument);
    EXPECT_THROW(p256PublicKeyOf(std::string(32, '\xff')), std::invalid_argument); // larger than the curve order
}

TEST(WebPushCrypto, PublicKeyCheck)
{
    const P256KeyPair pair = generateP256KeyPair();
    EXPECT_EQ(pair.privateKey.size(), 32u);
    EXPECT_EQ(p256PublicKeyOf(pair.privateKey), pair.publicKey);

    EXPECT_TRUE(isP256PublicKey(pair.publicKey));
    EXPECT_FALSE(isP256PublicKey(pair.publicKey.substr(0, 64)));
    std::string offCurve = pair.publicKey;
    offCurve[64] = static_cast<char>(offCurve[64] ^ 1);
    EXPECT_FALSE(isP256PublicKey(offCurve));
    EXPECT_FALSE(isP256PublicKey(std::string(65, '\x04')));
}

TEST(WebPushCrypto, VapidJwtIsSignedWithTheServerKey)
{
    const P256KeyPair server = generateP256KeyPair();
    const std::string jwt = vapidJwt("https://fcm.googleapis.com", 1800000000, "https://ubermenschs.com",
                                     server.privateKey);

    const std::size_t firstDot = jwt.find('.');
    const std::size_t secondDot = jwt.rfind('.');
    ASSERT_NE(firstDot, secondDot);
    EXPECT_EQ(base64UrlDecode(jwt.substr(0, firstDot)), R"({"typ":"JWT","alg":"ES256"})");
    EXPECT_EQ(base64UrlDecode(jwt.substr(firstDot + 1, secondDot - firstDot - 1)),
              R"({"aud":"https://fcm.googleapis.com","exp":1800000000,"sub":"https://ubermenschs.com"})");

    const std::string signature = base64UrlDecode(jwt.substr(secondDot + 1));
    ASSERT_EQ(signature.size(), 64u);
    EXPECT_TRUE(verifyEs256(jwt.substr(0, secondDot), signature, server.publicKey));
    // Another key cannot have made it.
    EXPECT_FALSE(verifyEs256(jwt.substr(0, secondDot), signature, generateP256KeyPair().publicKey));
}

/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <gtest/gtest.h>
#include <memory>

#include <acsdk/CodecUtils/Hex.h>
#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/Crypto/private/OpenSslCryptoCodec.h>
#include <acsdk/Crypto/private/OpenSslKeyFactory.h>

namespace alexaClientSDK {
namespace crypto {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::cryptoInterfaces;
using namespace alexaClientSDK::codecUtils;

const std::string TEST_STR = "The quick brown fox jumps over the lazy dog";        // 43 characters
const std::string TEST_STR2 = "The quick brown fox jumps over the lazy dog.....";  // 48 characters

/// @private
static constexpr AlgorithmType BAD_ALGORITHM_TYPE = static_cast<AlgorithmType>(0);

static std::vector<unsigned char> hexStringToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    decodeHex(hex, bytes);
    return bytes;
}

static std::string bytesToHexString(const std::vector<unsigned char>& bytes) {
    std::string result;
    encodeHex(bytes, result);
    return result;
}

static std::vector<unsigned char> stringToBytes(const std::string& str) {
    std::vector<unsigned char> bytes((const unsigned char*)str.data(), (const unsigned char*)str.data() + str.size());
    return bytes;
}

const OpenSslCryptoCodec::Key ZERO_KEY =
    hexStringToBytes("0000000000000000000000000000000000000000000000000000000000000000");
/// Zero IV.
const OpenSslCryptoCodec::IV IV0 = hexStringToBytes("00000000000000000000000000000000");
/// Random IV.
const OpenSslCryptoCodec::IV IVR = hexStringToBytes("0123456789abcdef0123456789abcdef");
/// Bad IV.
const OpenSslCryptoCodec::IV IVB = hexStringToBytes("0123456789");
/// Test string encrypted with AES-256-CBC-PAD with IVR.
const std::string AES256CBCPAD_CIPHERTEXT_IVR =
    "0df523194582f51a623a9ad0395d5ed62f7880b70e14818f7648fb01999bca27f955aac7e15dff71944d952de2ca9e99";
/// Test string encrypted with AES-256-CBC-PAD with IV0.
const std::string AES256CBCPAD_CIPHERTEXT_IV0 =
    "6db0c67c0cf728b37640f65f0e7db88f5cd217822b08cbad8817dda0f19476684d05a1b1c6a7b5184510b3a0e43b552a";
/// Test string 2 encrypted with AES-256-CBC with IVR.
const std::string AES256CBC_CIPHERTEXT_IVR =
    "0df523194582f51a623a9ad0395d5ed62f7880b70e14818f7648fb01999bca27cd24efc62c1b96e0c14b661d4ef5cdf9";
/// Test string 2 encrypted with AES-256-CBC with IV0.
const std::string AES256CBC_CIPHERTEXT_IV0 =
    "6db0c67c0cf728b37640f65f0e7db88f5cd217822b08cbad8817dda0f194766832570123a3c6dd75c19fd304f9321b6f";

static std::string bytesToString(const std::vector<unsigned char>& bytes) {
    std::string result(bytes.data(), bytes.data() + bytes.size());
    return result;
}

TEST(OpenSslCryptoCodecTest, test_badAlgorithmEncoder) {
    auto encoder = OpenSslCryptoCodec::createEncoder(BAD_ALGORITHM_TYPE);
    ASSERT_EQ(nullptr, encoder);
}

TEST(OpenSslCryptoCodecTest, test_badAlgorithmDecoder) {
    auto decoder = OpenSslCryptoCodec::createDecoder(BAD_ALGORITHM_TYPE);
    ASSERT_EQ(nullptr, decoder);
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcEncoder) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC);
    ASSERT_NE(nullptr, encoder);
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcPadEncoder) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, encoder);
}

/**
 * Test fixture for generic parameter error tests.
 *
 * @private
 */
class GenericCodecTest : public testing::TestWithParam<AlgorithmType> {
public:
    /// Allocates key factory.
    static void SetUpTestCase() {
        c_keyFactory = OpenSslKeyFactory::create();
    }

    /// Releases key factory.
    static void TearDownTestCase() {
        c_keyFactory.reset();
    }

    // Key factory used in tests.
    static std::shared_ptr<KeyFactoryInterface> c_keyFactory;
};
std::shared_ptr<KeyFactoryInterface> GenericCodecTest::c_keyFactory;

TEST_P(GenericCodecTest, test_encodeNoInit) {
    auto encoder = OpenSslCryptoCodec::createEncoder(GetParam());
    ASSERT_NE(nullptr, encoder);
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_FALSE(encoder->process({}, encoded));
}

TEST_P(GenericCodecTest, test_decodeNoInit) {
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    ASSERT_NE(nullptr, decoder);
    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_FALSE(decoder->process({}, decoded));
}

TEST_P(GenericCodecTest, test_encodeFinalizeNoInit) {
    auto encoder = OpenSslCryptoCodec::createEncoder(GetParam());
    ASSERT_NE(nullptr, encoder);
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_FALSE(encoder->finalize(encoded));
}

TEST_P(GenericCodecTest, test_decodeFinalizeNoInit) {
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    ASSERT_NE(nullptr, decoder);
    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_FALSE(decoder->finalize(decoded));
}

TEST_P(GenericCodecTest, test_encoderInitBadIV) {
    auto encoder = OpenSslCryptoCodec::createEncoder(GetParam());
    ASSERT_NE(nullptr, encoder);
    CryptoCodecInterface::Key key;
    ASSERT_TRUE(c_keyFactory->generateKey(GetParam(), key));
    ASSERT_FALSE(encoder->init(key, IVB));
}

TEST_P(GenericCodecTest, test_decoderInitBadIV) {
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::Key key;
    ASSERT_TRUE(c_keyFactory->generateKey(GetParam(), key));
    ASSERT_FALSE(decoder->init(key, IVB));
}

TEST_P(GenericCodecTest, test_encoderInitBadKey) {
    auto encoder = OpenSslCryptoCodec::createEncoder(GetParam());
    ASSERT_NE(nullptr, encoder);
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateIV(GetParam(), iv));
    ASSERT_FALSE(encoder->init(IVB, iv));
}

TEST_P(GenericCodecTest, test_decoderInitBadKey) {
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateIV(GetParam(), iv));
    ASSERT_FALSE(decoder->init(IVB, iv));
}

TEST_P(GenericCodecTest, test_encodeDecodeEmpty) {
    auto encoder = OpenSslCryptoCodec::createEncoder(GetParam());
    ASSERT_NE(nullptr, encoder);
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::Key key;
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateKey(GetParam(), key));
    ASSERT_TRUE(c_keyFactory->generateIV(GetParam(), iv));
    ASSERT_TRUE(encoder->init(key, iv));
    ASSERT_TRUE(decoder->init(key, iv));

    CryptoCodecInterface::DataBlock plaintext;
    CryptoCodecInterface::DataBlock ciphertext;
    CryptoCodecInterface::DataBlock plaintext2;

    ASSERT_TRUE(encoder->process(plaintext, ciphertext));
    ASSERT_TRUE(encoder->finalize(ciphertext));

    ASSERT_TRUE(decoder->process(ciphertext, plaintext2));
    ASSERT_TRUE(decoder->finalize(plaintext2));

    ASSERT_TRUE(plaintext2.empty());
}

TEST_P(GenericCodecTest, test_encodeDecodeNonEmpty) {
    auto encoder = OpenSslCryptoCodec::createEncoder(GetParam());
    ASSERT_NE(nullptr, encoder);
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::Key key;
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateKey(GetParam(), key));
    ASSERT_TRUE(c_keyFactory->generateIV(GetParam(), iv));
    ASSERT_TRUE(encoder->init(key, iv));
    ASSERT_TRUE(decoder->init(key, iv));

    CryptoCodecInterface::DataBlock plaintext;
    plaintext.assign(TEST_STR2.data(), TEST_STR2.data() + TEST_STR2.size());
    CryptoCodecInterface::DataBlock ciphertext;
    CryptoCodecInterface::DataBlock plaintext2;

    ASSERT_TRUE(encoder->process(plaintext, ciphertext));
    ASSERT_TRUE(encoder->finalize(ciphertext));
    ASSERT_FALSE(ciphertext.empty());
    ASSERT_EQ(0u, ciphertext.size() % 16);

    ASSERT_TRUE(decoder->process(ciphertext, plaintext2));
    ASSERT_TRUE(decoder->finalize(plaintext2));

    ASSERT_EQ(plaintext, plaintext2);
}

TEST_P(GenericCodecTest, test_decodeEmptyError) {
    auto decoder = OpenSslCryptoCodec::createDecoder(GetParam());
    CryptoCodecInterface::Key key;
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateKey(GetParam(), key));
    ASSERT_TRUE(c_keyFactory->generateIV(GetParam(), iv));
    ASSERT_TRUE(decoder->init(key, iv));

    CryptoCodecInterface::DataBlock ciphertext;
    CryptoCodecInterface::DataBlock plaintext;

    ASSERT_TRUE(decoder->process(ciphertext, plaintext));
    ASSERT_TRUE(plaintext.empty());

    switch (GetParam()) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
            ASSERT_TRUE(decoder->finalize(plaintext));
            ASSERT_TRUE(plaintext.empty());
            break;
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            ASSERT_FALSE(decoder->finalize(plaintext));
            break;
        default:
            ASSERT_FALSE(true);
            break;
    }
}

INSTANTIATE_TEST_CASE_P(
    OpenSslCryptoCodecTest,
    GenericCodecTest,
    Values(
        AlgorithmType::AES_256_CBC,
        AlgorithmType::AES_256_CBC_PAD,
        AlgorithmType::AES_128_CBC,
        AlgorithmType::AES_128_CBC_PAD));

TEST(OpenSslCryptoCodecTest, test_aes256CbcPadEncodeEmpty) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, encoder);
    ASSERT_TRUE(encoder->init(ZERO_KEY, IV0));
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process({}, encoded));
    ASSERT_TRUE(encoded.empty());
    ASSERT_TRUE(encoder->finalize(encoded));
    ASSERT_EQ(16u, encoded.size());
    ASSERT_EQ("1f788fe6d86c317549697fbf0c07fa43", bytesToHexString(encoded));
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcEncodeZeroIV) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC);
    ASSERT_NE(nullptr, encoder);
    ASSERT_TRUE(encoder->init(ZERO_KEY, IV0));
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process(stringToBytes(TEST_STR2), encoded));
    ASSERT_TRUE(encoder->finalize(encoded));
    ASSERT_EQ(AES256CBC_CIPHERTEXT_IV0, bytesToHexString(encoded));
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcPadEncodeZeroIV) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, encoder);
    ASSERT_TRUE(encoder->init(ZERO_KEY, IV0));
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process(stringToBytes(TEST_STR), encoded));
    ASSERT_TRUE(encoder->finalize(encoded));
    ASSERT_EQ(AES256CBCPAD_CIPHERTEXT_IV0, bytesToHexString(encoded));
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcEncodeNonEmptyIV) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC);
    ASSERT_NE(nullptr, encoder);
    ASSERT_TRUE(encoder->init(ZERO_KEY, IVR));
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process(stringToBytes(TEST_STR2), encoded));
    ASSERT_TRUE(encoder->finalize(encoded));
    ASSERT_EQ(AES256CBC_CIPHERTEXT_IVR, bytesToHexString(encoded));
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcPadEncodeNonEmptyIV) {
    auto encoder = OpenSslCryptoCodec::createEncoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, encoder);
    ASSERT_EQ(true, encoder->init(ZERO_KEY, IVR));
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process(stringToBytes(TEST_STR), encoded));
    ASSERT_TRUE(encoder->finalize(encoded));
    ASSERT_EQ(AES256CBCPAD_CIPHERTEXT_IVR, bytesToHexString(encoded));
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcPadDecodeEmptyError) {
    auto decoder = OpenSslCryptoCodec::createDecoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, decoder);
    ASSERT_TRUE(decoder->init(ZERO_KEY, IV0));
    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_TRUE(decoder->process({}, decoded));
    ASSERT_TRUE(decoded.empty());
    // We expect an error.
    ASSERT_FALSE(decoder->finalize(decoded));
}

TEST(OpenSslCryptoCodecTest, test_aes256CbcDecodeString) {
    auto decoder = OpenSslCryptoCodec::createDecoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, decoder);
    ASSERT_TRUE(decoder->init(ZERO_KEY, IVR));
    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_TRUE(decoder->process(hexStringToBytes(AES256CBCPAD_CIPHERTEXT_IVR), decoded));
    ASSERT_TRUE(decoder->finalize(decoded));

    ASSERT_EQ(TEST_STR, bytesToString(decoded));
}

}  // namespace test
}  // namespace crypto
}  // namespace alexaClientSDK

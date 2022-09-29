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
#include <tuple>

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

static std::vector<unsigned char> hexStringToBytes(const std::string& hex);

/// Test string for encryption and decryption.
static const std::string TEST_STR{"The quick brown fox jumps over the lazy dog"};
/// Test authentication data for encryption and decryption.
static const std::string TEST_AD{"Authentication data"};
/// Initialization vector.
const OpenSslCryptoCodec::IV TEST_IV = hexStringToBytes("0EB033BB783123FBA5391E94");
/// AES-128 bit key.
const OpenSslCryptoCodec::Key TEST_KEY128 = hexStringToBytes("3595292D00F5F379C231DD785609C3F1");
/// AES-256 bit key.
const OpenSslCryptoCodec::Key TEST_KEY256 =
    hexStringToBytes("829E7C69986F36F0F3116F3D3F9E941839193C3849D6CCCCA42AA734792A7081");
/// MAC for encrypting @a TEST_STR with @a TEST_KEY128 and @a TEST_IV.
const std::string TEST_TAG128{"0554a0cb6e9d120b041a246c0376b02b"};
/// MAC for encrypting @a TEST_STR with @a TEST_KEY256 and @a TEST_IV.
const std::string TEST_TAG256{"d79fbdd28e70ff74f267301f51c2471e"};
/// Random MAC code.
const std::string TEST_TAGBAD{"00000000000000000000000000000000"};
/// Ciphertext from encrypting @a TEST_STR with @a TEST_KEY128 and @a TEST_IV.
const std::string TEST_CIPHERTEXT128{
    "40d7b2a1e750f8e3d731424f7536b4a113b77ca248c3356075d3a9cfedcd7fae84ea2d7983e86f9581833f"};
/// Ciphertext from encrypting @a TEST_STR with @a TEST_KEY256 and @a TEST_IV.
const std::string TEST_CIPHERTEXT256{
    "f940a05f273315d1fae75e4fc68f401848051231d7c20319ea7efaa7eb6166b56fcfb790056fc84a912050"};

// Helper function to convert hex string to byte vector.
static std::vector<unsigned char> hexStringToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    decodeHex(hex, bytes);
    return bytes;
}

// Helper function to convert byte vector to hex string.
static std::string bytesToHexString(const std::vector<unsigned char>& bytes) {
    std::string hexString;
    encodeHex(bytes, hexString);
    return hexString;
}

// Helper function to represent string as a byte vector.
static std::vector<unsigned char> stringToBytes(const std::string& str) {
    std::vector<unsigned char> bytes(
        reinterpret_cast<const unsigned char*>(str.data()),
        reinterpret_cast<const unsigned char*>(str.data()) + str.size());
    return bytes;
}

// Helper function to represent byte vector as a string.
static std::string bytesToString(const std::vector<unsigned char>& bytes) {
    std::string result(
        reinterpret_cast<const char*>(bytes.data()), reinterpret_cast<const char*>(bytes.data()) + bytes.size());
    return result;
}

/// Test parameter type.
///
/// Tests take algorithm type, key, tag, and ciphertext as input.
typedef std::tuple<AlgorithmType, const OpenSslCryptoCodec::Key&, const std::string&, const std::string&> TestParams;

/**
 * Test fixture for generic parameter error tests.
 *
 * Parameters include: algorithm type, used key, expected tag, and expected ciphertext.
 * @private
 */
class AeAdCodecTest : public testing::TestWithParam<TestParams> {
public:
    /// Allocates key factory.
    static void SetUpTestCase() {
        c_keyFactory = OpenSslKeyFactory::create();
    }

    /// Releases key factory.
    static void TearDownTestCase() {
        c_keyFactory.reset();
    }

    // Key factory used in some tests.
    static std::shared_ptr<KeyFactoryInterface> c_keyFactory;
};
std::shared_ptr<KeyFactoryInterface> AeAdCodecTest::c_keyFactory;

TEST_P(AeAdCodecTest, test_encodeNoInit) {
    AlgorithmType algorithmType;
    std::tie(algorithmType, std::ignore, std::ignore, std::ignore) = GetParam();
    auto encoder = OpenSslCryptoCodec::createEncoder(algorithmType);
    ASSERT_NE(nullptr, encoder);
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_FALSE(encoder->process({}, encoded));
}

TEST_P(AeAdCodecTest, test_decodeNoInit) {
    AlgorithmType algorithmType;
    std::tie(algorithmType, std::ignore, std::ignore, std::ignore) = GetParam();
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);
    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_FALSE(decoder->process({}, decoded));
}

TEST_P(AeAdCodecTest, test_encodeFinalizeNoInit) {
    AlgorithmType algorithmType;
    std::tie(algorithmType, std::ignore, std::ignore, std::ignore) = GetParam();
    auto encoder = OpenSslCryptoCodec::createEncoder(algorithmType);
    ASSERT_NE(nullptr, encoder);
    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_FALSE(encoder->finalize(encoded));
}

TEST_P(AeAdCodecTest, test_decodeFinalizeNoInit) {
    AlgorithmType algorithmType;
    std::tie(algorithmType, std::ignore, std::ignore, std::ignore) = GetParam();
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);
    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_FALSE(decoder->finalize(decoded));
}

TEST_P(AeAdCodecTest, test_encodeDecodeEmpty) {
    AlgorithmType algorithmType;
    std::tie(algorithmType, std::ignore, std::ignore, std::ignore) = GetParam();
    auto encoder = OpenSslCryptoCodec::createEncoder(algorithmType);
    ASSERT_NE(nullptr, encoder);
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::Key key;
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateKey(algorithmType, key));
    ASSERT_TRUE(c_keyFactory->generateIV(algorithmType, iv));
    ASSERT_TRUE(encoder->init(key, iv));
    ASSERT_TRUE(decoder->init(key, iv));

    CryptoCodecInterface::DataBlock plaintext;
    CryptoCodecInterface::DataBlock ciphertext;
    CryptoCodecInterface::DataBlock plaintext2;

    ASSERT_TRUE(encoder->process(plaintext, ciphertext));
    ASSERT_TRUE(encoder->finalize(ciphertext));

    CryptoCodecInterface::Tag tag;
    ASSERT_TRUE(encoder->getTag(tag));
    ASSERT_EQ(16u, tag.size());

    ASSERT_TRUE(decoder->process(ciphertext, plaintext2));
    ASSERT_TRUE(decoder->setTag(tag));
    ASSERT_TRUE(decoder->finalize(plaintext2));

    ASSERT_TRUE(plaintext2.empty());
}

TEST_P(AeAdCodecTest, test_encodeDecodeNonEmpty) {
    AlgorithmType algorithmType;
    std::tie(algorithmType, std::ignore, std::ignore, std::ignore) = GetParam();
    auto encoder = OpenSslCryptoCodec::createEncoder(algorithmType);
    ASSERT_NE(nullptr, encoder);
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::Key key;
    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(c_keyFactory->generateKey(algorithmType, key));
    ASSERT_TRUE(c_keyFactory->generateIV(algorithmType, iv));
    ASSERT_TRUE(encoder->init(key, iv));
    ASSERT_TRUE(decoder->init(key, iv));

    CryptoCodecInterface::DataBlock plaintext = stringToBytes(TEST_STR);
    CryptoCodecInterface::DataBlock ciphertext;
    CryptoCodecInterface::DataBlock plaintext2;
    CryptoCodecInterface::Tag tag;

    ASSERT_TRUE(encoder->processAAD(stringToBytes(TEST_AD)));
    ASSERT_TRUE(encoder->process(plaintext, ciphertext));
    ASSERT_TRUE(encoder->finalize(ciphertext));
    ASSERT_TRUE(encoder->getTag(tag));
    ASSERT_FALSE(ciphertext.empty());

    ASSERT_TRUE(decoder->processAAD(stringToBytes(TEST_AD)));
    ASSERT_TRUE(decoder->process(ciphertext, plaintext2));
    ASSERT_TRUE(decoder->setTag(tag));
    ASSERT_TRUE(decoder->finalize(plaintext2));

    ASSERT_EQ(TEST_STR, bytesToString(plaintext2));
}

TEST_P(AeAdCodecTest, test_encodeAadAfterProcess) {
    AlgorithmType algorithmType;
    CryptoCodecInterface::Key key;
    std::tie(algorithmType, key, std::ignore, std::ignore) = GetParam();
    auto encoder = OpenSslCryptoCodec::createEncoder(algorithmType);
    ASSERT_NE(nullptr, encoder);

    ASSERT_TRUE(encoder->init(key, TEST_IV));

    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process(stringToBytes(TEST_STR), encoded));
    ASSERT_FALSE(encoder->processAAD(stringToBytes(TEST_AD)));
}

TEST_P(AeAdCodecTest, test_decodeAadAfterProcess) {
    AlgorithmType algorithmType;
    CryptoCodecInterface::Key key;
    std::string ciphertext;
    std::tie(algorithmType, key, std::ignore, ciphertext) = GetParam();
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);

    ASSERT_TRUE(decoder->init(key, TEST_IV));

    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_TRUE(decoder->process(stringToBytes(ciphertext), decoded));
    ASSERT_FALSE(decoder->processAAD(stringToBytes(TEST_AD)));
}

TEST_P(AeAdCodecTest, test_encodeTestData) {
    AlgorithmType algorithmType;
    CryptoCodecInterface::Key key;
    std::string tag;
    std::string ciphertext;
    std::tie(algorithmType, key, tag, ciphertext) = GetParam();
    auto encoder = OpenSslCryptoCodec::createEncoder(algorithmType);
    ASSERT_NE(nullptr, encoder);

    ASSERT_TRUE(encoder->init(key, TEST_IV));
    ASSERT_TRUE(encoder->processAAD(stringToBytes(TEST_AD)));

    OpenSslCryptoCodec::DataBlock encoded;
    ASSERT_TRUE(encoder->process(stringToBytes(TEST_STR), encoded));
    ASSERT_TRUE(encoder->finalize(encoded));
    ASSERT_EQ(ciphertext, bytesToHexString(encoded));

    OpenSslCryptoCodec::Tag tag2;
    ASSERT_TRUE(encoder->getTag(tag2));
    ASSERT_EQ(tag, bytesToHexString(tag2));
}

TEST_P(AeAdCodecTest, test_decodeTestData) {
    AlgorithmType algorithmType;
    CryptoCodecInterface::Key key;
    std::string tag;
    std::string ciphertext;
    std::tie(algorithmType, key, tag, ciphertext) = GetParam();
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);

    ASSERT_TRUE(decoder->init(key, TEST_IV));
    ASSERT_TRUE(decoder->processAAD(stringToBytes(TEST_AD)));

    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_TRUE(decoder->process(hexStringToBytes(ciphertext), decoded));
    ASSERT_TRUE(decoder->setTag(hexStringToBytes(tag)));
    ASSERT_TRUE(decoder->finalize(decoded));
    ASSERT_EQ(TEST_STR, bytesToString(decoded));
}

TEST_P(AeAdCodecTest, test_decodeStringWrongTag) {
    AlgorithmType algorithmType;
    CryptoCodecInterface::Key key;
    std::string ciphertext;
    std::tie(algorithmType, key, std::ignore, ciphertext) = GetParam();
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);

    ASSERT_TRUE(decoder->init(key, TEST_IV));
    ASSERT_TRUE(decoder->processAAD(stringToBytes(TEST_AD)));

    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_TRUE(decoder->process(hexStringToBytes(ciphertext), decoded));
    ASSERT_TRUE(decoder->setTag(hexStringToBytes(TEST_TAGBAD)));
    ASSERT_FALSE(decoder->finalize(decoded));
}

TEST_P(AeAdCodecTest, test_decodeStringNoTag) {
    AlgorithmType algorithmType;
    CryptoCodecInterface::Key key;
    std::string ciphertext;
    std::tie(algorithmType, key, std::ignore, ciphertext) = GetParam();
    auto decoder = OpenSslCryptoCodec::createDecoder(algorithmType);
    ASSERT_NE(nullptr, decoder);

    ASSERT_TRUE(decoder->init(key, TEST_IV));
    ASSERT_TRUE(decoder->processAAD(stringToBytes(TEST_AD)));

    OpenSslCryptoCodec::DataBlock decoded;
    ASSERT_TRUE(decoder->process(hexStringToBytes(ciphertext), decoded));
    ASSERT_FALSE(decoder->finalize(decoded));
}

INSTANTIATE_TEST_CASE_P(
    OpenSslCryptoCodecAEADTest,
    AeAdCodecTest,
    Values(
        TestParams{AlgorithmType::AES_256_GCM, TEST_KEY256, TEST_TAG256, TEST_CIPHERTEXT256},
        TestParams{AlgorithmType::AES_128_GCM, TEST_KEY128, TEST_TAG128, TEST_CIPHERTEXT128}));

}  // namespace test
}  // namespace crypto
}  // namespace alexaClientSDK

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

#include <acsdk/CodecUtils/Hex.h>
#include <acsdk/Crypto/private/OpenSslDigest.h>

namespace alexaClientSDK {
namespace crypto {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::cryptoInterfaces;
using namespace ::alexaClientSDK::codecUtils;

static const char TEST_STR[] = "The quick brown fox jumps over the lazy dog";
static const std::vector<unsigned char> TEST_DATA{TEST_STR, TEST_STR + sizeof(TEST_STR) - 1};
static const uint8_t TEST_UINT8 = 1;
static const uint16_t TEST_UINT16 = 1;
static const uint32_t TEST_UINT32 = 1;
static const uint64_t TEST_UINT64 = 1;
static const std::string SHA256_EMPTY_HEX{"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
static const std::string SHA256_TEST_DATA_HEX{"d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"};
// To verify: echo 01 | xxd -r -p | openssl dgst -sha256
static const std::string SHA256_UINT8_HEX{"4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"};
// To verify: echo 0001 | xxd -r -p | openssl dgst -sha256
static const std::string SHA256_UNT16_HEX{"b413f47d13ee2fe6c845b2ee141af81de858df4ec549a58b7970bb96645bc8d2"};
// To verify: echo 00000001 | xxd -r -p | openssl dgst -sha256
static const std::string SHA256_UINT32_HEX{"b40711a88c7039756fb8a73827eabe2c0fe5a0346ca7e0a104adc0fc764f528d"};
// To verify: echo 0000000000000001 | xxd -r -p | openssl dgst -sha256
static const std::string SHA256_UINT64_HEX{"cd2662154e6d76b2b2b92e70c0cac3ccf534f9b74eb5b89819ec509083d00a50"};

static const std::string MD5_EMPTY_HEX{"d41d8cd98f00b204e9800998ecf8427e"};
static const std::string MD5_TEST_DATA_HEX{"9e107d9d372bb6826bd81d3542a419d6"};
// To verify: echo 01 | xxd -r -p | openssl dgst -md5
static const std::string MD5_UINT8_HEX{"55a54008ad1ba589aa210d2629c1df41"};
// To verify: echo 0001 | xxd -r -p | openssl dgst -md5
static const std::string MD5_UNT16_HEX{"441077cc9e57554dd476bdfb8b8b8102"};
// To verify: echo 00000001 | xxd -r -p | openssl dgst -md5
static const std::string MD5_UINT32_HEX{"f1450306517624a57eafbbf8ed995985"};
// To verify: echo 0000000000000001 | xxd -r -p | openssl dgst -md5
static const std::string MD5_UINT64_HEX{"fa5ad9a8557e5a84cf23e52d3d3adf77"};

static constexpr DigestType BAD_DIGEST_TYPE = static_cast<DigestType>(0);

/**
 * A test struct to encapsulate  @c DigestType and @c hexString value.
 *
 * @private
 */
struct OpenSslDigestTestData {
    OpenSslDigestTestData(DigestType digestType, const std::string& hexString) :
            m_digestType(digestType),
            m_hexString(hexString) {
    }

    DigestType m_digestType;
    std::string m_hexString;
};

/**
 * Test fixture for create digest tests.
 *
 * @private
 */

class OpenSslDigestCreateFixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestCreateFixture, test_create) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    ASSERT_NE(nullptr, digest);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestCreateFixture,
    ::testing::Values(OpenSslDigestTestData(DigestType::SHA_256, ""), OpenSslDigestTestData(DigestType::MD5, "")));

TEST(OpenSslDigestTest, test_createInvalid) {
    auto digest = OpenSslDigest::create(BAD_DIGEST_TYPE);
    ASSERT_EQ(nullptr, digest);
}

/**
 * Test fixture for empty digest tests.
 *
 * @private
 */
class OpenSslDigestEmptyFixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestEmptyFixture, test_emptyDigest) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    digest->process({});
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(openSslDigestTestData.m_hexString, hex);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestEmptyFixture,
    ::testing::Values(
        OpenSslDigestTestData(DigestType::SHA_256, SHA256_EMPTY_HEX),
        OpenSslDigestTestData(DigestType::MD5, MD5_EMPTY_HEX)));

/**
 * Test fixture for testdata digest tests.
 *
 * @private
 */
class OpenSslDigestTestDataFixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestTestDataFixture, test_digest) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    digest->process(TEST_DATA);
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(openSslDigestTestData.m_hexString, hex);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestTestDataFixture,
    ::testing::Values(
        OpenSslDigestTestData(DigestType::SHA_256, SHA256_TEST_DATA_HEX),
        OpenSslDigestTestData(DigestType::MD5, MD5_TEST_DATA_HEX)));

/**
 * Test fixture for UInt8 digest tests.
 *
 * @private
 */
class OpenSslDigestUInt8Fixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestUInt8Fixture, test_digestUInt8) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    ASSERT_TRUE(digest->processUInt8(TEST_UINT8));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(openSslDigestTestData.m_hexString, hex);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestUInt8Fixture,
    ::testing::Values(
        OpenSslDigestTestData(DigestType::SHA_256, SHA256_UINT8_HEX),
        OpenSslDigestTestData(DigestType::MD5, MD5_UINT8_HEX)));

/**
 * Test fixture for UInt16 digest tests.
 *
 * @private
 */
class OpenSslDigestUInt16Fixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestUInt16Fixture, test_digestUInt16) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    ASSERT_TRUE(digest->processUInt16(TEST_UINT16));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(openSslDigestTestData.m_hexString, hex);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestUInt16Fixture,
    ::testing::Values(
        OpenSslDigestTestData(DigestType::SHA_256, SHA256_UNT16_HEX),
        OpenSslDigestTestData(DigestType::MD5, MD5_UNT16_HEX)));

/**
 * Test fixture for UInt32 digest tests.
 *
 * @private
 */
class OpenSslDigestUInt32Fixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestUInt32Fixture, test_digestUInt32) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    ASSERT_TRUE(digest->processUInt32(TEST_UINT32));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(openSslDigestTestData.m_hexString, hex);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestUInt32Fixture,
    ::testing::Values(
        OpenSslDigestTestData(DigestType::SHA_256, SHA256_UINT32_HEX),
        OpenSslDigestTestData(DigestType::MD5, MD5_UINT32_HEX)));

/**
 * Test fixture for UInt64 digest tests.
 *
 * @private
 */
class OpenSslDigestUInt64Fixture : public testing::TestWithParam<OpenSslDigestTestData> {};

TEST_P(OpenSslDigestUInt64Fixture, test_digestUInt64) {
    OpenSslDigestTestData openSslDigestTestData = GetParam();
    auto digest = OpenSslDigest::create(openSslDigestTestData.m_digestType);
    ASSERT_TRUE(digest->processUInt64(TEST_UINT64));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(openSslDigestTestData.m_hexString, hex);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    OpenSslDigestUInt64Fixture,
    ::testing::Values(
        OpenSslDigestTestData(DigestType::SHA_256, SHA256_UINT64_HEX),
        OpenSslDigestTestData(DigestType::MD5, MD5_UINT64_HEX)));

}  // namespace test
}  // namespace crypto
}  // namespace alexaClientSDK

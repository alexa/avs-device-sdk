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

#include <acsdkCodecUtils/Hex.h>
#include <acsdkCrypto/private/OpenSslDigest.h>

namespace alexaClientSDK {
namespace acsdkCrypto {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::acsdkCryptoInterfaces;
using namespace ::alexaClientSDK::acsdkCodecUtils;

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
static constexpr DigestType BAD_DIGEST_TYPE = static_cast<DigestType>(0);

TEST(OpenSslDigestTest, test_createSHA256) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    ASSERT_NE(nullptr, digest);
}

TEST(OpenSslDigestTest, test_createInvalid) {
    auto digest = OpenSslDigest::create(BAD_DIGEST_TYPE);
    ASSERT_EQ(nullptr, digest);
}

TEST(OpenSslDigestTest, test_emptySha256Digest) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    digest->process({});
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(SHA256_EMPTY_HEX, hex);
}

TEST(OpenSslDigestTest, test_digest) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    digest->process(TEST_DATA);
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(SHA256_TEST_DATA_HEX, hex);
}

TEST(OpenSslDigestTest, test_digestUInt8) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    ASSERT_TRUE(digest->processUInt8(TEST_UINT8));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(SHA256_UINT8_HEX, hex);
}

TEST(OpenSslDigestTest, test_digestUInt16) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    ASSERT_TRUE(digest->processUInt16(TEST_UINT16));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(SHA256_UNT16_HEX, hex);
}

TEST(OpenSslDigestTest, test_digestUInt32) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    ASSERT_TRUE(digest->processUInt32(TEST_UINT32));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(SHA256_UINT32_HEX, hex);
}

TEST(OpenSslDigestTest, test_digestUInt64) {
    auto digest = OpenSslDigest::create(DigestType::SHA_256);
    ASSERT_TRUE(digest->processUInt64(TEST_UINT64));
    OpenSslDigest::DataBlock res;
    ASSERT_TRUE(digest->finalize(res));
    std::string hex;
    ASSERT_TRUE(encodeHex(res, hex));
    ASSERT_EQ(SHA256_UINT64_HEX, hex);
}

}  // namespace test
}  // namespace acsdkCrypto
}  // namespace alexaClientSDK

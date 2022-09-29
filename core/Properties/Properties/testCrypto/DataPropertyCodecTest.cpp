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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

#include <acsdk/CodecUtils/Hex.h>
#include <acsdk/CryptoInterfaces/test/MockCryptoFactory.h>
#include <acsdk/CryptoInterfaces/test/MockDigest.h>
#include <acsdk/Properties/private/DataPropertyCodec.h>

// Workaround for GCC < 5.x
#if (defined(__GNUC__) && (__GNUC___ < 5))
#define RETURN_UNIQUE_PTR(x) std::move(x)
#else
#define RETURN_UNIQUE_PTR(x) (x)
#endif

namespace alexaClientSDK {
namespace properties {
namespace test {

using namespace ::testing;

using namespace ::alexaClientSDK::codecUtils;
using namespace ::alexaClientSDK::cryptoInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces::test;

/// @private
static const DataPropertyCodec::IV TEST_IV{0x10, 0x10, 0x10, 0x10};
/// @private
static const DataPropertyCodec::DataBlock TEST_DATA_CIPHERTEXT{0xAA, 0xAA, 0xAA, 0xAA};
/// @private
static const DataPropertyCodec::DataBlock TEST_DATA_TAG{0x05, 0x05};
/// @private
static const DigestInterface::DataBlock TEST_DIGEST{0xDD, 0xDD};
/// @private
static const DigestInterface::DataBlock TEST_DIGEST2{0xEE, 0xEE};
/// @private
static const std::string TEST_DER_DIGEST_HEX{"301630100404101010100404aaaaaaaa040205050402dddd"};
/// @private
static const std::string TEST_DER_DIGEST2_HEX{"301630100404101010100404aaaaaaaa040205050402eeee"};

TEST(DataPropertyCodecTest, test_encodeDer) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();

    EXPECT_CALL(*mockCryptoFactory, createDigest(DigestType::SHA_256))
        .WillOnce(Invoke([](DigestType type) -> std::unique_ptr<DigestInterface> {
            auto mockDigest = std::unique_ptr<MockDigest>(new MockDigest);
            EXPECT_CALL(*mockDigest, process(_)).WillRepeatedly(Return(true));
            EXPECT_CALL(*mockDigest, finalize(_)).WillOnce(Invoke([](DigestInterface::DataBlock& res) -> bool {
                res.insert(res.end(), TEST_DIGEST.begin(), TEST_DIGEST.end());
                return true;
            }));
            return RETURN_UNIQUE_PTR(mockDigest);
        }));

    std::vector<uint8_t> derEncoded;
    ASSERT_TRUE(DataPropertyCodec::encode(mockCryptoFactory, TEST_IV, TEST_DATA_CIPHERTEXT, TEST_DATA_TAG, derEncoded));

    std::string hexDer;
    EXPECT_TRUE(encodeHex(derEncoded, hexDer));
    ASSERT_EQ(TEST_DER_DIGEST_HEX, hexDer);
}

TEST(DataPropertyCodecTest, test_decodeDer) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();

    EXPECT_CALL(*mockCryptoFactory, createDigest(DigestType::SHA_256))
        .WillRepeatedly(Invoke([](DigestType type) -> std::unique_ptr<DigestInterface> {
            auto mockDigest = std::unique_ptr<MockDigest>(new MockDigest);
            EXPECT_CALL(*mockDigest, process(_)).WillRepeatedly(Return(true));
            EXPECT_CALL(*mockDigest, finalize(_)).WillOnce(Invoke([](DigestInterface::DataBlock& res) -> bool {
                res.insert(res.end(), TEST_DIGEST2.begin(), TEST_DIGEST2.end());
                return true;
            }));
            return RETURN_UNIQUE_PTR(mockDigest);
        }));

    DataPropertyCodec::IV dataKeyIV;
    DataPropertyCodec::DataBlock dataKeyCiphertext;
    DataPropertyCodec::Tag dataKeyTag;
    DataPropertyCodec::DataBlock digestDecoded, digestActual;

    DataPropertyCodec::DataBlock derEncoded;
    decodeHex(TEST_DER_DIGEST_HEX, derEncoded);
    ASSERT_TRUE(DataPropertyCodec::decode(
        mockCryptoFactory, derEncoded, dataKeyIV, dataKeyCiphertext, dataKeyTag, digestDecoded, digestActual));

    ASSERT_EQ(TEST_IV, dataKeyIV);
    ASSERT_EQ(TEST_DATA_CIPHERTEXT, dataKeyCiphertext);
    ASSERT_EQ(TEST_DATA_TAG, dataKeyTag);

    ASSERT_TRUE(DataPropertyCodec::encode(mockCryptoFactory, dataKeyIV, dataKeyCiphertext, dataKeyTag, derEncoded));

    std::string hexDer;
    EXPECT_TRUE(encodeHex(derEncoded, hexDer));
    EXPECT_EQ(TEST_DER_DIGEST2_HEX, hexDer);
}

}  // namespace test
}  // namespace properties
}  // namespace alexaClientSDK

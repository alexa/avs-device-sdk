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
#include <acsdk/Properties/private/EncryptionKeyPropertyCodec.h>

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

using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces::test;
using namespace ::alexaClientSDK::codecUtils;

// Test content for decoding.
static const std::string DER_ENCODED_HEX =
    "3024301e0c076d61696e4b657904030303030404101010100404aaaaaaaa040205050402dddd";

TEST(EncryptionKeyPropertyTest, test_encodeDer) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();

    EXPECT_CALL(*mockCryptoFactory, createDigest(DigestType::SHA_256))
        .WillOnce(Invoke([](DigestType type) -> std::unique_ptr<DigestInterface> {
            auto mockDigest = std::unique_ptr<MockDigest>(new MockDigest);
            EXPECT_CALL(*mockDigest, process(_)).WillRepeatedly(Return(true));
            EXPECT_CALL(*mockDigest, finalize(_)).WillOnce(Invoke([](DigestInterface::DataBlock& res) -> bool {
                DigestInterface::DataBlock digest{0xDD, 0xDD};
                res.insert(res.end(), digest.begin(), digest.end());
                return true;
            }));
            return RETURN_UNIQUE_PTR(mockDigest);
        }));

    std::string mainKeyAlias = "mainKey";
    KeyStoreInterface::KeyChecksum mainKeyChecksum{0x03, 0x03, 0x03};
    AlgorithmType dataKeyAlgorithm = AlgorithmType::AES_256_GCM;
    KeyStoreInterface::IV dataKeyIV{
        0x10,
        0x10,
        0x10,
        0x10,
    };
    KeyStoreInterface::DataBlock dataKeyCiphertext{0xAA, 0xAA, 0xAA, 0xAA};
    KeyStoreInterface::Tag dataKeyTag{0x05, 0x05};
    AlgorithmType dataAlgorithm = AlgorithmType::AES_256_GCM;
    PropertiesInterface::Bytes derEncoded;
    ASSERT_TRUE(EncryptionKeyPropertyCodec::encode(
        mockCryptoFactory,
        mainKeyAlias,
        mainKeyChecksum,
        dataKeyAlgorithm,
        dataKeyIV,
        dataKeyCiphertext,
        dataKeyTag,
        dataAlgorithm,
        derEncoded));

    std::string hexString;
    ASSERT_TRUE(encodeHex(derEncoded, hexString));
    ASSERT_EQ(DER_ENCODED_HEX, hexString);
}

TEST(EncryptionKeyPropertyTest, test_decodeDer) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();

    EXPECT_CALL(*mockCryptoFactory, createDigest(DigestType::SHA_256))
        .WillRepeatedly(Invoke([](DigestType type) -> std::unique_ptr<DigestInterface> {
            auto mockDigest = std::unique_ptr<MockDigest>(new MockDigest);
            EXPECT_CALL(*mockDigest, process(_)).WillRepeatedly(Return(true));
            EXPECT_CALL(*mockDigest, finalize(_)).WillOnce(Invoke([](std::vector<uint8_t>& res) -> bool {
                DigestInterface::DataBlock digest{0xEE, 0xEE};
                res.insert(res.end(), digest.begin(), digest.end());
                return true;
            }));
            return RETURN_UNIQUE_PTR(mockDigest);
        }));

    std::string mainKeyAlias;
    KeyStoreInterface::KeyChecksum mainKeyChecksum;
    AlgorithmType dataKeyAlgorithm;
    KeyStoreInterface::IV dataKeyIV;
    KeyStoreInterface::DataBlock dataKeyCiphertext;
    KeyStoreInterface::Tag dataKeyTag;
    AlgorithmType dataAlgorithm;
    DigestInterface::DataBlock digestDecoded, digestActual;

    PropertiesInterface::Bytes derEncoded;

    ASSERT_TRUE(decodeHex(DER_ENCODED_HEX, derEncoded));
    ASSERT_TRUE(EncryptionKeyPropertyCodec::decode(
        mockCryptoFactory,
        derEncoded,
        mainKeyAlias,
        mainKeyChecksum,
        dataKeyAlgorithm,
        dataKeyIV,
        dataKeyCiphertext,
        dataKeyTag,
        dataAlgorithm,
        digestDecoded,
        digestActual));

    ASSERT_EQ("mainKey", mainKeyAlias);
    ASSERT_EQ((KeyStoreInterface::KeyChecksum{0x03, 0x03, 0x03}), mainKeyChecksum);
    ASSERT_EQ(AlgorithmType::AES_256_GCM, dataKeyAlgorithm);
    ASSERT_EQ(
        (KeyStoreInterface::IV{
            0x10,
            0x10,
            0x10,
            0x10,
        }),
        dataKeyIV);
    ASSERT_EQ((KeyStoreInterface::DataBlock{0xAA, 0xAA, 0xAA, 0xAA}), dataKeyCiphertext);
    ASSERT_EQ((KeyStoreInterface::Tag{0x05, 0x05}), dataKeyTag);
    ASSERT_EQ(AlgorithmType::AES_256_GCM, dataAlgorithm);

    ASSERT_TRUE(EncryptionKeyPropertyCodec::encode(
        mockCryptoFactory,
        mainKeyAlias,
        mainKeyChecksum,
        dataKeyAlgorithm,
        dataKeyIV,
        dataKeyCiphertext,
        dataKeyTag,
        dataAlgorithm,
        derEncoded));
}

}  // namespace test
}  // namespace properties
}  // namespace alexaClientSDK

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
#include <acsdk/Crypto/CryptoFactory.h>
#include <acsdk/Crypto/private/OpenSslCryptoFactory.h>

namespace alexaClientSDK {
namespace crypto {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::cryptoInterfaces;
using namespace ::alexaClientSDK::codecUtils;

/// Test IV for AES.
/// @private
static const std::string TEST_IV_HEX{"19100e18da95041e1c373806ba809254"};

/// Test key for AES 256.
/// @private
static const std::string TEST_KEY_HEX{"9afdf8f0d042299300c9dc50e7363c34ed5f4a78f4066163574e7d2641365855"};

/// Test plaintext to encrypt.
/// @private
static const std::string TEST_PLAINTEXT{"some plaintext value"};

/// Encrypted plaintext in hex form. This is \a TEST_PLAINTEXT value encrypted with \a TEST_KEY_HEX key and
/// \a TEST_IV_HEX IV usign AES-256-CBC with PKCS#7 padding.
/// @private
static const std::string TEST_CIPHERTEXT{"f3fa1a4bef50e2f55f3caa49fad568fe1c33fe8c7a66aadd6527c15dffc0a77a"};

/// Bad crypto algorithm type
/// @private
static constexpr AlgorithmType BAD_ALGORITHM_TYPE = static_cast<AlgorithmType>(0);

/// Bad digest type
/// @private
static constexpr DigestType BAD_DIGEST_TYPE = static_cast<DigestType>(0);

TEST(OpenSslCryptoFactoryTest, test_createNotNull) {
    auto factory = OpenSslCryptoFactory::create();
    ASSERT_NE(nullptr, factory);
}

TEST(OpenSslCryptoFactoryTest, test_createTools) {
    auto factory = OpenSslCryptoFactory::create();
    ASSERT_NE(nullptr, factory->createDecoder(AlgorithmType::AES_256_CBC));
    ASSERT_NE(nullptr, factory->createEncoder(AlgorithmType::AES_256_CBC));
    ASSERT_NE(nullptr, factory->createDecoder(AlgorithmType::AES_256_CBC_PAD));
    ASSERT_NE(nullptr, factory->createEncoder(AlgorithmType::AES_256_CBC_PAD));
    ASSERT_NE(nullptr, factory->createDecoder(AlgorithmType::AES_128_CBC));
    ASSERT_NE(nullptr, factory->createEncoder(AlgorithmType::AES_128_CBC));
    ASSERT_NE(nullptr, factory->createDecoder(AlgorithmType::AES_256_CBC_PAD));
    ASSERT_NE(nullptr, factory->createEncoder(AlgorithmType::AES_256_CBC_PAD));
    ASSERT_NE(nullptr, factory->createDecoder(AlgorithmType::AES_128_GCM));
    ASSERT_NE(nullptr, factory->createEncoder(AlgorithmType::AES_128_GCM));
    ASSERT_NE(nullptr, factory->createDecoder(AlgorithmType::AES_256_GCM));
    ASSERT_NE(nullptr, factory->createEncoder(AlgorithmType::AES_256_GCM));
    ASSERT_NE(nullptr, factory->getKeyFactory());
    ASSERT_NE(nullptr, factory->createDigest(DigestType::SHA_256));
}

TEST(OpenSslCryptoFactoryTest, test_createUnknownTools) {
    auto factory = OpenSslCryptoFactory::create();
    ASSERT_EQ(nullptr, factory->createDecoder(BAD_ALGORITHM_TYPE));
    ASSERT_EQ(nullptr, factory->createEncoder(BAD_ALGORITHM_TYPE));
    ASSERT_EQ(nullptr, factory->createDigest(BAD_DIGEST_TYPE));
}

TEST(OpenSslCryptoFactoryTest, test_encryptDecrypt) {
    auto cryptoFactory = createCryptoFactory();
    ASSERT_NE(nullptr, cryptoFactory);

    CryptoCodecInterface::IV iv;
    ASSERT_TRUE(decodeHex(TEST_IV_HEX, iv));
    CryptoCodecInterface::Key key;
    ASSERT_TRUE(decodeHex(TEST_KEY_HEX, key));

    std::string text{TEST_PLAINTEXT};
    CryptoCodecInterface::DataBlock plaintext;
    plaintext.assign(text.data(), text.data() + text.size());

    CryptoCodecInterface::DataBlock ciphertext;
    auto encoder = cryptoFactory->createEncoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, encoder);
    ASSERT_TRUE(encoder->init(key, iv));
    ASSERT_TRUE(encoder->process(plaintext, ciphertext));
    ASSERT_TRUE(encoder->finalize(ciphertext));

    std::string ciphertextStr;
    ASSERT_TRUE(encodeHex(ciphertext, ciphertextStr));
    ASSERT_EQ(TEST_CIPHERTEXT, ciphertextStr);

    auto decoder = cryptoFactory->createDecoder(AlgorithmType::AES_256_CBC_PAD);
    ASSERT_NE(nullptr, decoder);
    CryptoCodecInterface::DataBlock plaintext2;
    ASSERT_TRUE(decoder->init(key, iv));
    ASSERT_TRUE(decoder->process(ciphertext, plaintext2));
    ASSERT_TRUE(decoder->finalize(plaintext2));

    ASSERT_EQ(plaintext, plaintext2);
}

}  // namespace test
}  // namespace crypto
}  // namespace alexaClientSDK

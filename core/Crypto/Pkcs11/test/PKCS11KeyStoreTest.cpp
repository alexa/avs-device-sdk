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

#include <string>

#include <gtest/gtest.h>

#include <acsdk/Pkcs11/private/PKCS11Functions.h>
#include <acsdk/Pkcs11/private/PKCS11Slot.h>
#include <acsdk/Pkcs11/private/PKCS11KeyStore.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace pkcs11 {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;
using namespace ::alexaClientSDK::cryptoInterfaces;

/// @private
static const KeyStoreInterface::IV
    IV{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
/// @private
static const KeyStoreInterface::DataBlock PLAINTEXT{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'i', 'o', 'p', 'q', 'r', 's', 't', 'u',
};
/// @private
static constexpr AlgorithmType BAD_ALGORITHM_TYPE = AlgorithmType(0);

/// @private
// clang-format off
static const std::string JSON_TEST_CONFIG =
    "{\"pkcs11Module\":{\"libraryPath\":\"" PKCS11_LIBRARY "\",\"tokenName\":\"" PKCS11_TOKEN_NAME
    "\","
    "\"userPin\":\"" PKCS11_PIN "\",\"defaultKeyName\":\"" PKCS11_KEY_NAME "\"}}";
// clang-format on

/// @private
static void initConfig() {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>(JSON_TEST_CONFIG);
    ConfigurationNode::initialize({ss});
}

TEST(PKCS11KeyStoreTest, test_create) {
    initConfig();
    auto keyStore = PKCS11KeyStore::create();
    ASSERT_NE(nullptr, keyStore);
}

TEST(PKCS11KeyStoreTest, test_createBadConfig) {
    ConfigurationNode::uninitialize();
    auto keyStore = PKCS11KeyStore::create();
    ASSERT_EQ(nullptr, keyStore);
}

TEST(PKCS11KeyStoreTest, test_encryptDecrypt) {
    initConfig();
    auto keyStore = PKCS11KeyStore::create();
    ASSERT_NE(nullptr, keyStore);
    KeyStoreInterface::DataBlock ciphertext;
    KeyStoreInterface::KeyChecksum checksum;

    ASSERT_TRUE(keyStore->encrypt(PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC, IV, PLAINTEXT, checksum, ciphertext));
    ASSERT_NE(PLAINTEXT, ciphertext);
    KeyStoreInterface::DataBlock plaintext;
    ASSERT_TRUE(keyStore->decrypt(PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC, checksum, IV, ciphertext, plaintext));
    ASSERT_EQ(PLAINTEXT, plaintext);
}

TEST(PKCS11KeyStoreTest, test_encryptWithWrongAlgorithm) {
    initConfig();
    auto keyStore = PKCS11KeyStore::create();
    ASSERT_NE(nullptr, keyStore);
    KeyStoreInterface::DataBlock ciphertext;
    KeyStoreInterface::KeyChecksum checksum;
    ASSERT_FALSE(keyStore->encrypt(PKCS11_KEY_NAME, BAD_ALGORITHM_TYPE, IV, PLAINTEXT, checksum, ciphertext));
}

TEST(PKCS11KeyStoreTest, test_decryptWithWrongAlgorithm) {
    initConfig();
    auto keyStore = PKCS11KeyStore::create();
    ASSERT_NE(nullptr, keyStore);
    KeyStoreInterface::DataBlock ciphertext;
    KeyStoreInterface::KeyChecksum checksum;
    ASSERT_TRUE(keyStore->encrypt(PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC, IV, PLAINTEXT, checksum, ciphertext));

    ASSERT_NE(PLAINTEXT, ciphertext);
    KeyStoreInterface::DataBlock plaintext;
    ASSERT_FALSE(keyStore->decrypt(PKCS11_KEY_NAME, BAD_ALGORITHM_TYPE, IV, checksum, ciphertext, plaintext));
}

TEST(PKCS11KeyStoreTest, test_createOrLoadKeyTwiceUsesTheSameKey) {
    initConfig();
    auto keyStore = PKCS11KeyStore::create();
    ASSERT_NE(nullptr, keyStore);
    KeyStoreInterface::DataBlock ciphertext1;
    KeyStoreInterface::KeyChecksum checksum1;
    ASSERT_TRUE(keyStore->encrypt(PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC, IV, PLAINTEXT, checksum1, ciphertext1));
    KeyStoreInterface::DataBlock ciphertext2;
    KeyStoreInterface::KeyChecksum checksum2;
    ASSERT_TRUE(keyStore->encrypt(PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC, IV, PLAINTEXT, checksum2, ciphertext2));
    ASSERT_EQ(ciphertext1, ciphertext2);
    ASSERT_EQ(checksum1, checksum2);
}

}  // namespace test
}  // namespace pkcs11
}  // namespace alexaClientSDK

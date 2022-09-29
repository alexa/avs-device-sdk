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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdk/Pkcs11/private/PKCS11Functions.h>
#include <acsdk/Pkcs11/private/PKCS11Slot.h>
#include <acsdk/Pkcs11/private/PKCS11Session.h>
#include <acsdk/Pkcs11/private/PKCS11Key.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>

namespace alexaClientSDK {
namespace pkcs11 {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::cryptoInterfaces;

/// @private
static const KeyStoreInterface::IV
    IV{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
/// @private
static const KeyStoreInterface::IV IV_GCM{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};
/// @private
static const KeyStoreInterface::DataBlock PLAINTEXT{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'i', 'o', 'p', 'q', 'r', 's', 't', 'u',
};

TEST(PKCS11KeyTest, test_encryptDecryptAes256Cbc) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName(PKCS11_TOKEN_NAME, slot));
    ASSERT_NE(nullptr, slot);
    auto session = slot->openSession();
    ASSERT_NE(nullptr, session);
    ASSERT_TRUE(session->logIn(PKCS11_PIN));

    auto key = session->findKey({PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC});
    ASSERT_NE(nullptr, key);

    KeyStoreInterface::Tag tag;
    KeyStoreInterface::DataBlock aad;

    // Encrypt
    KeyStoreInterface::DataBlock ciphertext;
    ASSERT_TRUE(key->encrypt(AlgorithmType::AES_256_CBC, IV, aad, PLAINTEXT, ciphertext, tag));

    EXPECT_NE(PLAINTEXT, ciphertext);

    // Decrypt
    KeyStoreInterface::DataBlock plaintext;
    ASSERT_TRUE(key->decrypt(AlgorithmType::AES_256_CBC, IV, aad, ciphertext, tag, plaintext));

    EXPECT_EQ(PLAINTEXT, plaintext);
    key.reset();
    ASSERT_TRUE(session->logOut());
}

TEST(PKCS11KeyTest, test_encryptDecryptErrors) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName(PKCS11_TOKEN_NAME, slot));
    ASSERT_NE(nullptr, slot);
    auto session = slot->openSession();
    ASSERT_NE(nullptr, session);
    ASSERT_TRUE(session->logIn(PKCS11_PIN));

    auto key = session->findKey({PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC});
    ASSERT_NE(nullptr, key);

    KeyStoreInterface::Tag tag;
    KeyStoreInterface::DataBlock aad;

    // Encrypt with bad IV
    std::vector<uint8_t> output;
    ASSERT_FALSE(key->encrypt(AlgorithmType::AES_256_CBC, aad, {}, PLAINTEXT, output, tag));

    // Decrypt with bad IV
    ASSERT_FALSE(key->decrypt(AlgorithmType::AES_256_CBC, aad, {}, PLAINTEXT, output, tag));
}

TEST(PKCS11KeyTest, test_encryptDecryptAes256CbcPad) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName(PKCS11_TOKEN_NAME, slot));
    ASSERT_NE(nullptr, slot);
    auto session = slot->openSession();
    ASSERT_NE(nullptr, session);
    ASSERT_TRUE(session->logIn(PKCS11_PIN));
    auto key = session->findKey({PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC_PAD});
    ASSERT_NE(nullptr, key);

    KeyStoreInterface::Tag tag;
    KeyStoreInterface::DataBlock aad;

    // Encrypt
    KeyStoreInterface::DataBlock ciphertext;
    ASSERT_TRUE(key->encrypt(AlgorithmType::AES_256_CBC_PAD, IV, aad, PLAINTEXT, ciphertext, tag));
    EXPECT_NE(PLAINTEXT, ciphertext);

    // Decrypt
    KeyStoreInterface::DataBlock plaintext;
    ASSERT_TRUE(key->decrypt(AlgorithmType::AES_256_CBC_PAD, IV, aad, ciphertext, tag, plaintext));
    EXPECT_EQ(PLAINTEXT, plaintext);

    ASSERT_TRUE(session->logOut());
}

TEST(PKCS11KeyTest, test_encryptDecryptAes256Gcm) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName(PKCS11_TOKEN_NAME, slot));
    ASSERT_NE(nullptr, slot);
    auto session = slot->openSession();
    ASSERT_NE(nullptr, session);
    ASSERT_TRUE(session->logIn(PKCS11_PIN));
    auto key = session->findKey({PKCS11_KEY_NAME, AlgorithmType::AES_256_GCM});
    ASSERT_NE(nullptr, key);

    KeyStoreInterface::Tag tag;
    KeyStoreInterface::DataBlock aad{0, 1, 2};

    // Encrypt
    KeyStoreInterface::DataBlock ciphertext;
    ASSERT_TRUE(key->encrypt(AlgorithmType::AES_256_GCM, IV_GCM, aad, PLAINTEXT, ciphertext, tag));
    EXPECT_NE(PLAINTEXT, ciphertext);
    ASSERT_EQ(16u, tag.size());

    // Decrypt
    KeyStoreInterface::DataBlock plaintext;
    ASSERT_TRUE(key->decrypt(AlgorithmType::AES_256_GCM, IV_GCM, aad, ciphertext, tag, plaintext));
    EXPECT_EQ(PLAINTEXT, plaintext);

    ASSERT_TRUE(session->logOut());
}

TEST(PKCS11KeyTest, test_getKeyAttributes) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName(PKCS11_TOKEN_NAME, slot));
    ASSERT_NE(nullptr, slot);
    auto session = slot->openSession();
    ASSERT_NE(nullptr, session);
    ASSERT_TRUE(session->logIn(PKCS11_PIN));
    auto key = session->findKey({PKCS11_KEY_NAME, AlgorithmType::AES_256_CBC_PAD});
    ASSERT_NE(nullptr, key);
    KeyStoreInterface::KeyChecksum checksum;
    bool neverExtractable = false;
    ASSERT_TRUE(key->getAttributes(checksum, neverExtractable));
    ASSERT_EQ(3u, checksum.size());
    ASSERT_TRUE(neverExtractable);
}

}  // namespace test
}  // namespace pkcs11
}  // namespace alexaClientSDK

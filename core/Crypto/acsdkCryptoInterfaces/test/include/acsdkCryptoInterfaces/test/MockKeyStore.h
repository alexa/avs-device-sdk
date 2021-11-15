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

#ifndef ACSDKCRYPTOINTERFACES_TEST_MOCKKEYSTORE_H_
#define ACSDKCRYPTOINTERFACES_TEST_MOCKKEYSTORE_H_

#include <acsdkCryptoInterfaces/KeyStoreInterface.h>
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace acsdkCryptoInterfaces {
namespace test {

/**
 * Mock class for @c KeyStoreInterface.
 */
class MockKeyStore : public KeyStoreInterface {
public:
    MOCK_METHOD6(
        _encrypt,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const IV& iv,
            const DataBlock& plaintext,
            KeyChecksum& checksum,
            DataBlock& ciphertext));
    MOCK_METHOD8(
        _encryptAE,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const IV& iv,
            const DataBlock& aad,
            const DataBlock& plaintext,
            KeyChecksum& checksum,
            DataBlock& ciphertext,
            DataBlock& tag));
    MOCK_METHOD6(
        _decrypt,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const KeyChecksum& checksum,
            const IV& iv,
            const DataBlock& ciphertext,
            DataBlock& plaintext));
    MOCK_METHOD8(
        _decryptAD,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const KeyChecksum& checksum,
            const IV& iv,
            const DataBlock& aad,
            const DataBlock& ciphertext,
            const Tag& tag,
            DataBlock& plaintext));
    MOCK_METHOD1(_getDefaultKeyAlias, bool(std::string&));

    bool encrypt(
        const std::string& keyAlias,
        AlgorithmType type,
        const IV& iv,
        const KeyChecksum& plaintext,
        DataBlock& checksum,
        DataBlock& ciphertext) noexcept override;
    bool encryptAE(
        const std::string& keyAlias,
        AlgorithmType type,
        const IV& iv,
        const DataBlock& aad,
        const DataBlock& plaintext,
        KeyChecksum& checksum,
        DataBlock& ciphertext,
        Tag& tag) noexcept override;
    bool decrypt(
        const std::string& keyAlias,
        AlgorithmType type,
        const KeyChecksum& checksum,
        const IV& iv,
        const DataBlock& ciphertext,
        DataBlock& plaintext) noexcept override;
    bool decryptAD(
        const std::string& keyAlias,
        AlgorithmType type,
        const KeyChecksum& checksum,
        const IV& iv,
        const DataBlock& aad,
        const DataBlock& ciphertext,
        const Tag& tag,
        DataBlock& plaintext) noexcept override;
    bool getDefaultKeyAlias(std::string& keyAlias) noexcept override;
};

inline bool MockKeyStore::encrypt(
    const std::string& keyAlias,
    AlgorithmType type,
    const IV& iv,
    const DataBlock& plaintext,
    KeyChecksum& checksum,
    DataBlock& ciphertext) noexcept {
    return _encrypt(keyAlias, type, iv, plaintext, checksum, ciphertext);
}

inline bool MockKeyStore::encryptAE(
    const std::string& keyAlias,
    AlgorithmType type,
    const IV& iv,
    const DataBlock& aad,
    const DataBlock& plaintext,
    KeyChecksum& checksum,
    DataBlock& ciphertext,
    Tag& tag) noexcept {
    return _encryptAE(keyAlias, type, iv, aad, plaintext, checksum, ciphertext, tag);
}

inline bool MockKeyStore::decrypt(
    const std::string& keyAlias,
    AlgorithmType type,
    const KeyChecksum& checksum,
    const IV& iv,
    const DataBlock& ciphertext,
    DataBlock& plaintext) noexcept {
    return _decrypt(keyAlias, type, checksum, iv, ciphertext, plaintext);
}

inline bool MockKeyStore::decryptAD(
    const std::string& keyAlias,
    AlgorithmType type,
    const KeyChecksum& checksum,
    const IV& iv,
    const DataBlock& aad,
    const DataBlock& ciphertext,
    const Tag& tag,
    DataBlock& plaintext) noexcept {
    return _decryptAD(keyAlias, type, checksum, iv, aad, ciphertext, tag, plaintext);
}

inline bool MockKeyStore::getDefaultKeyAlias(std::string& keyAlias) noexcept {
    return _getDefaultKeyAlias(keyAlias);
}

}  // namespace test
}  // namespace acsdkCryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTOINTERFACES_TEST_MOCKKEYSTORE_H_

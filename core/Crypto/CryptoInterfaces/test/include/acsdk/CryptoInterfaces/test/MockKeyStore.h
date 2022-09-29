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

#ifndef ACSDK_CRYPTOINTERFACES_TEST_MOCKKEYSTORE_H_
#define ACSDK_CRYPTOINTERFACES_TEST_MOCKKEYSTORE_H_

#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace cryptoInterfaces {
namespace test {

/**
 * Mock class for @c KeyStoreInterface.
 */
class MockKeyStore : public KeyStoreInterface {
public:
    MOCK_NOEXCEPT_METHOD6(
        encrypt,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const IV& iv,
            const DataBlock& plaintext,
            KeyChecksum& checksum,
            DataBlock& ciphertext));
    MOCK_NOEXCEPT_METHOD8(
        encryptAE,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const IV& iv,
            const DataBlock& aad,
            const DataBlock& plaintext,
            KeyChecksum& checksum,
            DataBlock& ciphertext,
            DataBlock& tag));
    MOCK_NOEXCEPT_METHOD6(
        decrypt,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const KeyChecksum& checksum,
            const IV& iv,
            const DataBlock& ciphertext,
            DataBlock& plaintext));
    MOCK_NOEXCEPT_METHOD8(
        decryptAD,
        bool(
            const std::string& keyAlias,
            AlgorithmType type,
            const KeyChecksum& checksum,
            const IV& iv,
            const DataBlock& aad,
            const DataBlock& ciphertext,
            const Tag& tag,
            DataBlock& plaintext));
    MOCK_NOEXCEPT_METHOD1(getDefaultKeyAlias, bool(std::string&));
};

}  // namespace test
}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_TEST_MOCKKEYSTORE_H_

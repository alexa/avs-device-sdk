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

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <acsdk/Crypto/private/Logging.h>
#include <acsdk/Crypto/private/OpenSslErrorCleanup.h>
#include <acsdk/Crypto/private/OpenSslKeyFactory.h>
#include <acsdk/Crypto/private/OpenSslTypeMapper.h>

namespace alexaClientSDK {
namespace crypto {

using namespace alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "OpenSSL::KeyFactory"

std::shared_ptr<KeyFactoryInterface> OpenSslKeyFactory::create() noexcept {
    return std::shared_ptr<OpenSslKeyFactory>(new OpenSslKeyFactory);
}

OpenSslKeyFactory::OpenSslKeyFactory() noexcept {
}

bool OpenSslKeyFactory::generateKey(AlgorithmType type, Key& key) noexcept {
    const EVP_CIPHER* evpCipher = OpenSslTypeMapper::mapAlgorithmToEvpCipher(type);
    if (!evpCipher) {
        ACSDK_ERROR(LX("cipherNotRecognized"));
        return false;
    }

    OpenSslErrorCleanup errorCleanup{TAG};
    int len = EVP_CIPHER_key_length(evpCipher);
    return generateRandom(key, len);
}

bool OpenSslKeyFactory::generateIV(AlgorithmType type, IV& iv) noexcept {
    const EVP_CIPHER* evpCipher = OpenSslTypeMapper::mapAlgorithmToEvpCipher(type);
    if (!evpCipher) {
        ACSDK_ERROR(LX("cipherNotRecognized"));
        return false;
    }

    OpenSslErrorCleanup errorCleanup{TAG};
    int len = EVP_CIPHER_iv_length(evpCipher);
    return generateRandom(iv, len);
}

bool OpenSslKeyFactory::generateRandom(std::vector<unsigned char>& data, int size) noexcept {
    if (size < 0) {
        ACSDK_ERROR(LX("negativeBlockSize"));
        return false;
    }
    data.resize(static_cast<size_t>(size));
    RAND_bytes(&data[0], size);
    return true;
}

}  // namespace crypto
}  // namespace alexaClientSDK

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
#include <acsdk/Crypto/private/OpenSslTypeMapper.h>

namespace alexaClientSDK {
namespace crypto {

using namespace alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "OpenSSL::TypeMapper"

const EVP_CIPHER* OpenSslTypeMapper::mapAlgorithmToEvpCipher(AlgorithmType type) noexcept {
    const EVP_CIPHER* evpCipher = nullptr;

    switch (type) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_256_CBC_PAD:
            evpCipher = EVP_aes_256_cbc();
            break;
        case AlgorithmType::AES_128_CBC:
        case AlgorithmType::AES_128_CBC_PAD:
            evpCipher = EVP_aes_128_cbc();
            break;
        case AlgorithmType::AES_128_GCM:
            evpCipher = EVP_aes_128_gcm();
            break;
        case AlgorithmType::AES_256_GCM:
            evpCipher = EVP_aes_256_gcm();
            break;
        default:
            ACSDK_ERROR(LX("unknownAlgorithmType").d("type", type));
            break;
    }

    return evpCipher;
}

bool OpenSslTypeMapper::mapAlgorithmToPadding(AlgorithmType type, PaddingMode& mode) noexcept {
    switch (type) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
            mode = PaddingMode::NoPadding;
            break;
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            mode = PaddingMode::Padding;
            break;
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            mode = PaddingMode::NoPadding;
            break;
        default:
            ACSDK_ERROR(LX("unknownAlgorithmType").d("type", type));
            return false;
    }

    return true;
}

bool OpenSslTypeMapper::mapAlgorithmToTagSize(AlgorithmType type, size_t& tagSize) noexcept {
    switch (type) {
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            tagSize = 16u;
            return true;
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            tagSize = 0u;
            return true;
        default:
            ACSDK_ERROR(LX("unknownAlgorithmType").d("type", type));
            return false;
    }
}

const EVP_MD* OpenSslTypeMapper::mapDigestToEvpMd(DigestType type) noexcept {
    const EVP_MD* evpMd = nullptr;

    switch (type) {
        case DigestType::SHA_256:
            evpMd = EVP_sha256();
            break;
        case DigestType::MD5:
            evpMd = EVP_md5();
            break;
        default:
            ACSDK_ERROR(LX("unknownAlgorithmType").d("type", type));
            break;
    }

    return evpMd;
}

}  // namespace crypto
}  // namespace alexaClientSDK

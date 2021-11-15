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

#ifndef ACSDKCRYPTO_PRIVATE_OPENSSLTYPEMAPPER_H_
#define ACSDKCRYPTO_PRIVATE_OPENSSLTYPEMAPPER_H_

#include <openssl/evp.h>

#include <acsdkCryptoInterfaces/AlgorithmType.h>
#include <acsdkCryptoInterfaces/DigestType.h>
#include <acsdkCrypto/private/OpenSslTypes.h>

namespace alexaClientSDK {
namespace acsdkCrypto {

using alexaClientSDK::acsdkCryptoInterfaces::AlgorithmType;
using alexaClientSDK::acsdkCryptoInterfaces::DigestType;

/**
 * @brief Helper class to map SDK types into types from OpenSSL EVP API.
 *
 * @ingroup CryptoIMPL
 */
class OpenSslTypeMapper {
public:
    /**
     * @brief Find OpenSSL codec implementation.
     *
     * Finds OpenSSL codec implementation for a given encryption algorithm.
     *
     * @param[in] type Encryption algorithm.
     *
     * @return OpenSSL algorithm reference or nullptr on error.
     */
    static const EVP_CIPHER* mapAlgorithmToEvpCipher(AlgorithmType type) noexcept;

    /**
     * @brief Determine padding mode for an encryption algorithm,
     *
     * Finds OpenSSL padding mode for a given encryption algorithm.
     *
     * @param[in]   type    Encryption Algorithm.
     * @param[out]  mode    Padding mode.
     *
     * @return Boolean indicating success.
     */
    static bool mapAlgorithmToPadding(AlgorithmType type, PaddingMode& mode) noexcept;

    /**
     * @brief Maps algorithm to tag size for AEAD algorithms.
     *
     * @param[in]   type    Encryption algorithm.
     * @param[out]  tagSize Tag size or 0 if algorithm doesn't support tags.
     *
     * @return Boolean indicating success.
     */
    static bool mapAlgorithmToTagSize(AlgorithmType type, size_t& tagSize) noexcept;

    /**
     * @brief Find OpenSSL digest implementation.
     *
     * Finds OpenSSL digest implementation for a given digest algorithm.
     *
     * @param[in] type Digest algorithm.
     *
     * @return OpenSSL algorithm reference or nullptr on error.
     */
    static const EVP_MD* mapDigestToEvpMd(DigestType type) noexcept;
};

}  // namespace acsdkCrypto
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTO_PRIVATE_OPENSSLTYPEMAPPER_H_

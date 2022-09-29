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

#ifndef ACSDK_CRYPTOINTERFACES_ALGORITHMTYPE_H_
#define ACSDK_CRYPTOINTERFACES_ALGORITHMTYPE_H_

#include <ostream>

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Enumeration of all supported encryption protocols.
 *
 * This enumeration defines cipher type, key size, mode of operation, and padding.
 *
 * @ingroup Lib_acsdkCryptoInterfaces
 */
enum class AlgorithmType {
    /// @brief AES 256 CBC.
    ///
    /// AES Encryption with 256 bit key size and cipher block chaining.
    AES_256_CBC = 1,

    /// @brief AES 256 CBC with Padding.
    ///
    /// AES Encryption with 256 bit key size, cipher block chaining, and PKCS#7 padding.
    AES_256_CBC_PAD = 2,

    /// @brief AES 128 CBC.
    ///
    /// AES Encryption with 128 bit key size and cipher block chaining.
    AES_128_CBC = 3,

    /// @brief AES 128 CBC with Padding.
    ///
    /// AES Encryption with 128 bit key size, cipher block chaining, and PKCS#7 padding.
    AES_128_CBC_PAD = 4,

    /// @brief AES 256 GCM.
    ///
    /// AES Encryption with 256 bit key size and Galois/Counter mode. This algorithm belongs to Authenticated Encryption
    /// / Authenticated Decryption (AEAD) family.
    AES_256_GCM = 5,

    /// @brief AES 128 GCM.
    ///
    /// AES Encryption with 128 bit key size and Galois/Counter mode. This algorithm belongs to Authenticated Encryption
    /// / Authenticated Decryption (AEAD) family.
    AES_128_GCM = 6,
};

/**
 * @brief Helper method to write algorithm type as a literal constant.
 *
 * This method enables logging functions to accept algorithm type as a value.
 *
 * @param[in] out   Output stream.
 * @param[in] value Value to write.
 *
 * @return Output stream.
 * @ingroup Lib_acsdkCryptoInterfaces
 */
std::ostream& operator<<(std::ostream& out, AlgorithmType value);

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_ALGORITHMTYPE_H_

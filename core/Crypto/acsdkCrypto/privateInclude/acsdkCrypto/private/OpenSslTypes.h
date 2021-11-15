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

#ifndef ACSDKCRYPTO_PRIVATE_OPENSSLTYPES_H_
#define ACSDKCRYPTO_PRIVATE_OPENSSLTYPES_H_

/**
 * @brief Macro for cutting off OpenSSL features introduced before 1.1.0 release.
 * @private
 * @ingroup CryptoIMPL
 */
#define OPENSSL_VERSION_NUMBER_1_1_0 0x10100000L

#include <ostream>

namespace alexaClientSDK {
namespace acsdkCrypto {

/**
 * @brief Typed enumeration for codec types to use with EVP API.
 *
 * This enumeration defines values for use with EVP_CipherInit_ex method.
 *
 * @ingroup CryptoIMPL
 */
enum class CodecType : int {
    Decoder = 0,  ///< Decoder.
    Encoder = 1,  ///< Encoder.
};

/**
 * @brief Typed enumeration for padding mode to use with EVP API.
 *
 * This enumeration defines values for use with EVP_CIPHER_CTX_set_padding method.
 *
 * @ingroup CryptoIMPL
 */
enum class PaddingMode : int {
    NoPadding = 0,  ///< No padding.
    Padding = 1,    ///< PKCS#7 padding.
};

/// Success code for some OpenSSL functions.
/// @private
/// @ingroup CryptoIMPL
static constexpr int OPENSSL_OK = 1;

}  // namespace acsdkCrypto
}  // namespace alexaClientSDK

namespace std {

/// @brief Pretty-print function for padding mode values.
/// @private
/// @ingroup CryptoIMPL
std::ostream& operator<<(std::ostream& o, alexaClientSDK::acsdkCrypto::PaddingMode mode);

}  // namespace std

#endif  // ACSDKCRYPTO_PRIVATE_OPENSSLTYPES_H_

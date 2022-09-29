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

#ifndef ACSDK_CRYPTO_PRIVATE_OPENSSLTYPES_H_
#define ACSDK_CRYPTO_PRIVATE_OPENSSLTYPES_H_

/**
 * @brief Macro for cutting off OpenSSL features introduced before 1.1.0 release.
 * @private
 * @ingroup Lib_acsdkCrypto
 */
#define OPENSSL_VERSION_NUMBER_1_1_0 0x10100000L

#include <ostream>

namespace alexaClientSDK {
namespace crypto {

/**
 * @brief Typed enumeration for codec types to use with EVP API.
 *
 * This enumeration defines values for use with EVP_CipherInit_ex method.
 *
 * @ingroup Lib_acsdkCrypto
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
 * @ingroup Lib_acsdkCrypto
 */
enum class PaddingMode : int {
    NoPadding = 0,  ///< No padding.
    Padding = 1,    ///< PKCS#7 padding.
};

/// Success code for some OpenSSL functions.
/// @private
/// @ingroup Lib_acsdkCrypto
static constexpr int OPENSSL_OK = 1;

}  // namespace crypto
}  // namespace alexaClientSDK

namespace std {

/// @brief Pretty-print function for padding mode values.
/// @private
/// @ingroup Lib_acsdkCrypto
std::ostream& operator<<(std::ostream& o, alexaClientSDK::crypto::PaddingMode mode);

}  // namespace std

#endif  // ACSDK_CRYPTO_PRIVATE_OPENSSLTYPES_H_

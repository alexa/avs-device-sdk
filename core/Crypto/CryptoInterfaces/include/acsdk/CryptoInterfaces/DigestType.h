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

#ifndef ACSDK_CRYPTOINTERFACES_DIGESTTYPE_H_
#define ACSDK_CRYPTOINTERFACES_DIGESTTYPE_H_

#include <ostream>

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Enumeration of all supported digest algorithms.
 *
 * This enumeration provides list of supported digest algorithms.
 *
 * @ingroup Lib_acsdkCryptoInterfaces
 */
enum class DigestType {
    /// @brief SHA-256 digest algorithm.
    ///
    /// Secure Hash Algorithm 2 with 256 bit digest.
    ///
    /// @see https://en.wikipedia.org/wiki/SHA-2
    SHA_256 = 1,

    /// @brief MD5 digest algorithm.
    ///
    /// Message Digest algorithm 5 with 128 bit digest.
    ///
    /// @see https://en.wikipedia.org/wiki/MD5
    MD5 = 2,
};

/**
 * @brief Helper method to write digest type as a literal constant.
 *
 * This method enables logging functions to accept digest type as a value.
 *
 * @param[in] out   Output stream.
 * @param[in] value Value to write.
 *
 * @return Output stream.
 * @ingroup Lib_acsdkCryptoInterfaces
 */
std::ostream& operator<<(std::ostream& out, DigestType value);

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_DIGESTTYPE_H_

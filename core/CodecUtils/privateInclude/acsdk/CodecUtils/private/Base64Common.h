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

#ifndef ACSDK_CODECUTILS_PRIVATE_BASE64COMMON_H_
#define ACSDK_CODECUTILS_PRIVATE_BASE64COMMON_H_

#include <string>

#include <acsdk/CodecUtils/Types.h>

namespace alexaClientSDK {
namespace codecUtils {

/// @brief Number of bytes in Base64 encoded block.
/// @private
static constexpr unsigned B64CHAR_BLOCK = 4;

/// @brief Number of bytes in Base64 binary block.
/// @private
static constexpr unsigned B64BIN_BLOCK = 3;

/**
 * @brief Preprocesses Base64 input for decoding.
 *
 * Method validates input string and strips all whitespace characters.
 *
 * @param[in] base64String Base64 string.
 * @param[out] output Binary form of base64 string without whitespaces.
 *
 * @return true on success, false on error.
 * @private
 */
bool preprocessBase64(const std::string& base64String, Bytes& output) noexcept;

}  // namespace codecUtils
}  // namespace alexaClientSDK

#endif  // ACSDK_CODECUTILS_PRIVATE_BASE64COMMON_H_

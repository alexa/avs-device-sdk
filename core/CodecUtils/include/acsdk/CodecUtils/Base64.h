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

#ifndef ACSDK_CODECUTILS_BASE64_H_
#define ACSDK_CODECUTILS_BASE64_H_

#include <string>
#include <acsdk/CodecUtils/Types.h>

namespace alexaClientSDK {
namespace codecUtils {

/**
 * @brief Encodes binary data into string using Base64.
 *
 * This method encodes binary data into printable form using Base64 encoding. The output uses characters A-Z,a-z,0-9,
 * "+", "/". Every three bytes of data are converted into four bytes of output. If the input is not a multiple of 3
 * bytes, the output will be padded with "=" characters (one or two).
 *
 * @param[in] binary Binary data to encode.
 * @param[in,out] base64String Destination container. The method appends data to the container.
 *
 * @return True, if operation succeeds. If operation fails, the contents of \a base64String is unmodified.
 *
 * @sa decodeBase64()
 * @ingroup CodecUtils
 */
bool encodeBase64(const Bytes& binary, std::string& base64String) noexcept;

/**
 * @brief Decodes binary data from string using Base64.
 *
 * This method decodes binary data from string using Base64. Whitespace, newline, and carriage return characters are
 * ignored.
 *
 * The method converts 4 input characters (excluding ignorable whitespace) into 3 output bytes. If the method encounters
 * unsupported character (other than A-Z,a-z,0-9,"+", "/", "=" at the end, or ignorable whitespace), the operation
 * fails.
 *
 * @param[in] base64String Data to decode in base64 form.
 * @param[in,out] binary Decoded data. The method appends data to the container.
 * @return True, if operation succeeds. If operation fails, the contents of \a binary is unmodified.
 *
 * @sa encodeBase64()
 * @ingroup CodecUtils
 */
bool decodeBase64(const std::string& base64String, Bytes& binary) noexcept;

}  // namespace codecUtils
}  // namespace alexaClientSDK

#endif  // ACSDK_CODECUTILS_BASE64_H_

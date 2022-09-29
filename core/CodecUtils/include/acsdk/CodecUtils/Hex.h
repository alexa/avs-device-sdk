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

#ifndef ACSDK_CODECUTILS_HEX_H_
#define ACSDK_CODECUTILS_HEX_H_

#include <string>
#include <acsdk/CodecUtils/Types.h>

namespace alexaClientSDK {
namespace codecUtils {

/**
 * Encode binary data into string using hex encoding.
 *
 * Method encodes binary data into hexadecimal printable form. Every input byte is represented by two output bytes.
 * The method uses number characters 0-9 and lowercase letters a-f to represent hexadecimal values.
 *
 * Method appends data to destination container.
 *
 * @param[in] binary Binary data to encode.
 * @param[in,out] hexString Container to store encoded data. The data is appended to container.
 *
 * @return True, if operation succeeds. If operation fails, the contents of \a hexString is unmodified.
 *
 * @sa decodeHex()
 * @ingroup CodecUtils
 */
bool encodeHex(const Bytes& binary, std::string& hexString) noexcept;

/**
 * @brief Decodes binary data from string using hex.
 *
 * Method decodes input from hexadecimal string. Whitespace, newline, and carriage return characters are ignored.
 *
 * The method converts every 2 input characters (excluding ignorable whitespace) into single output byte. If the method
 * encounters unsupported character (other than A-F,a-f,0-9, or ignorable whitespace), the operation fails.
 *
 * @param[in] hexString Data to decode in hex form.
 * @param[in,out] binary Container to store decoded data. The decoded contents is appended to container.
 *
 * @return True, if operation succeeds. If operation fails, the contents of \a binary is unmodified.
 *
 * @sa encodeHex()
 * @ingroup CodecUtils
 */
bool decodeHex(const std::string& hexString, Bytes& binary) noexcept;

}  // namespace codecUtils
}  // namespace alexaClientSDK

#endif  // ACSDK_CODECUTILS_HEX_H_

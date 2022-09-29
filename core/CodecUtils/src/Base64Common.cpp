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

#include <acsdk/CodecUtils/private/Base64Common.h>
#include <acsdk/CodecUtils/private/CodecsCommon.h>

namespace alexaClientSDK {
namespace codecUtils {

/**
 * @brief Checks if the character is valid.
 *
 * This method checks if the character is a valid for base64 decoding. The valid character must be one of A-Z,a-z,0-9,
 * '/','+'.
 *
 * @param[in] ch Character to test.
 * @return true if character is valid, false otherwise.
 * @private
 */
static bool isValidChar(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || '/' == ch || '+' == ch;
}

bool preprocessBase64(const std::string& base64String, Bytes& output) noexcept {
    // Strip all ignorable characters from input before processing and validate the input.
    // The input must match the expression:
    // ^([a-zA-Z0-9+/]{4})*([a-zA-Z0-9+/]{3}=|[a-zA-Z0-9+/]{2}==)?$
    output.reserve(base64String.size());
    unsigned int cnt = 0;
    bool seenTail = false;
    for (char ch : base64String) {
        if (isIgnorableWhitespace(ch)) {
            continue;
        }
        if (seenTail) {
            if (cnt < B64BIN_BLOCK || '=' != ch) {
                // Bad input: data after '=' character.
                return false;
            }
            output.push_back(ch);
        } else {
            if (isValidChar(ch)) {
                output.push_back(ch);
            } else if (cnt > 1 && ch == '=') {
                seenTail = true;
                output.push_back(ch);
            } else {
                return false;
            }
        }
        cnt = (cnt + 1) % B64CHAR_BLOCK;
    }
    if (output.empty()) {
        return true;
    }
    if (output.size() % B64CHAR_BLOCK) {
        return false;
    }
    return true;
}

}  // namespace codecUtils
}  // namespace alexaClientSDK

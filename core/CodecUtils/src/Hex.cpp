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

#include <acsdk/CodecUtils/Hex.h>
#include <acsdk/CodecUtils/private/CodecsCommon.h>

namespace alexaClientSDK {
namespace codecUtils {

/**
 * @brief Static table to convert binary code into ASCII character.
 *
 * This array maps values 0-15 into one of the 0-9,a-f characters.
 * @private
 */
static const char BINARY_TO_HEX[] = "0123456789abcdef";

bool encodeHex(const Bytes& binary, std::string& hexString) noexcept {
    hexString.reserve(hexString.size() + binary.size() * 2);

    for (Byte b : binary) {
        hexString.push_back(BINARY_TO_HEX[b >> 4]);
        hexString.push_back(BINARY_TO_HEX[b & 15u]);
    }

    return true;
}

/**
 * @brief Converts character code into binary.
 * This method converts hex-encoded code into binary form. The input must be one of 0-9,a-f,A-F characters.
 *
 * @param[in] ch Character to decode.
 * @return Binary code.
 * @private
 */
///
static int charToInt(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else /* if (ch >= 'A' && ch <= 'F') */ {
        return ch - 'A' + 10;
    }
}

/**
 * @brief Helper to verify if the character is valid for hex decoding.
 *
 * Method verifies if the input character is a valid hexadecimal symbol.
 *
 * @param[in] ch Character code to verify.
 * @retval true If \a ch is one of 0-9,a-f,A-F characters.
 * @private
 */
static bool isValidHexChar(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

bool decodeHex(const std::string& hexString, Bytes& binary) noexcept {
    binary.reserve(hexString.size() / 2 + binary.size());

    int b0 = 0;
    bool accumulator = false;

    // validate the input.
    for (char ch : hexString) {
        if (isIgnorableWhitespace(ch)) {
            continue;
        }
        if (!isValidHexChar(ch)) {
            return false;
        }
        accumulator = !accumulator;
    }
    if (accumulator) {
        // We have an odd number of input characters, which is an error.
        return false;
    }

    for (char ch : hexString) {
        if (isIgnorableWhitespace(ch)) {
            continue;
        }

        int b1 = charToInt(ch);

        if (accumulator) {
            accumulator = false;
            binary.push_back(static_cast<Byte>((b0 << 4) | b1));
        } else {
            accumulator = true;
            b0 = b1;
        }
    }

    return true;
}

}  // namespace codecUtils
}  // namespace alexaClientSDK

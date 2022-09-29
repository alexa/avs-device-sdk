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

#include <climits>

#include <acsdk/CodecUtils/private/Base64Common.h>

namespace alexaClientSDK {
namespace codecUtils {

/// @brief Number of bits in Base64 byte.
/// @private
static constexpr unsigned B64CHAR_BIT = 6;

/// @brief Bit mask (maximum value) of Base64 byte.
/// @private
static constexpr unsigned B64CHAR_MAX = 0x3F;

/// @brief Letter count for A-Z or a-z range.
/// @private
static constexpr unsigned AZ_LETTER_COUNT = 26;

/// @brief Letter count for 0-9 range.
/// @private
static constexpr unsigned DIGITS_COUNT = 10;

/// @brief Binary value for plus character.
/// @private
static constexpr unsigned SYM_PLUS_VALUE = 62;

/// @brief Binary value for divide character.
/// @private
static constexpr unsigned SYM_DIV_VALUE = 63;

/**
 * @brief Maps binary value into character.
 *
 * Maps 6-bit binary value into ASCII character for Base64 encoding.
 *
 * @param[in] value Binary value to map into character. The value must be in range of 0..63 inclusive.
 * @return ASCII character.
 */
static char mapValueToChar(uint32_t value) noexcept {
    if (value < AZ_LETTER_COUNT) {
        return static_cast<char>(value + 'A');
    } else if (value < AZ_LETTER_COUNT * 2) {
        return static_cast<char>(value - AZ_LETTER_COUNT + 'a');
    } else if (value < AZ_LETTER_COUNT * 2 + DIGITS_COUNT) {
        return static_cast<char>(value - (AZ_LETTER_COUNT * 2) + '0');
    } else if (SYM_PLUS_VALUE == value) {
        return '+';
    } else {
        // value must be equal to SYM_DIV_VALUE
        return '/';
    }
}

bool encodeBase64(const Bytes& binary, std::string& base64String) noexcept {
    if (binary.empty()) {
        return true;
    }

    size_t nBlocks = binary.size() / B64BIN_BLOCK;
    size_t nTail = binary.size() % B64BIN_BLOCK;

    size_t outputSize = nBlocks * B64CHAR_BLOCK;
    if (nTail) {
        outputSize += B64CHAR_BLOCK;
    }
    base64String.reserve(base64String.size() + outputSize);

    unsigned int accumulator = 0;
    unsigned int nBits = 0;

    for (Byte b : binary) {
        accumulator = (accumulator << CHAR_BIT) | static_cast<unsigned int>(b);
        nBits += CHAR_BIT;
        if (nBits == B64CHAR_BIT * 2) {
            nBits = B64CHAR_BIT;
            base64String.push_back(mapValueToChar((accumulator >> B64CHAR_BIT) & B64CHAR_MAX));
        }
        if (nBits >= B64CHAR_BIT) {
            nBits -= B64CHAR_BIT;
            base64String.push_back(mapValueToChar((accumulator >> nBits) & B64CHAR_MAX));
        }
    }
    if (nBits > 0) {
        base64String.push_back(mapValueToChar((accumulator << (B64CHAR_BIT - nBits)) & B64CHAR_MAX));
    }

    if (nTail) {
        base64String.push_back('=');
        if (1 == nTail) {
            base64String.push_back('=');
        }
    }

    return true;
}

bool decodeBase64(const std::string& base64String, Bytes& binary) noexcept {
    Bytes tmp;
    if (!preprocessBase64(base64String, tmp)) {
        return false;
    }
    if (tmp.empty()) {
        return true;
    }
    if (tmp.size() % B64CHAR_BLOCK) {
        return false;
    }
    size_t expectedLen = tmp.size() / B64CHAR_BLOCK * B64BIN_BLOCK;

    unsigned int accumulator = 0;
    unsigned int nBits = 0;
    size_t len = 0;
    for (unsigned char ch : tmp) {
        unsigned value;
        if (ch == '=') {
            break;
        } else if (ch >= 'A' && ch <= 'Z') {
            value = ch - 'A';
        } else if (ch >= 'a' && ch <= 'z') {
            value = ch - 'a' + AZ_LETTER_COUNT;
        } else if (ch >= '0' && ch <= '9') {
            value = ch - '0' + AZ_LETTER_COUNT * 2;
        } else if ('+' == ch) {
            value = SYM_PLUS_VALUE;
        } else {
            // ch must be equal to '/'
            value = SYM_DIV_VALUE;
        }
        accumulator = (accumulator << B64CHAR_BIT) | value;
        nBits += B64CHAR_BIT;
        if (nBits >= CHAR_BIT) {
            nBits -= CHAR_BIT;
            binary.push_back(static_cast<Byte>((accumulator >> nBits) & UCHAR_MAX));
            len++;
        }
    }

    return len <= expectedLen;
}

}  // namespace codecUtils
}  // namespace alexaClientSDK

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

#include <openssl/evp.h>

#include <acsdk/CodecUtils/private/Base64Common.h>

namespace alexaClientSDK {
namespace codecUtils {

bool encodeBase64(const Bytes& binary, std::string& base64String) noexcept {
    if (binary.empty()) {
        return true;
    }

    // Base64 creates 4 output bytes for each 3 bytes of input
    size_t expectedLen = binary.size() / B64BIN_BLOCK * B64CHAR_BLOCK;
    // If input size is not dividable by 3, Base64 creates an additional 4 byte block.
    if (binary.size() % B64BIN_BLOCK) {
        expectedLen += B64CHAR_BLOCK;
    }
    std::vector<char> tmp;
    // OpenSSL encoder adds a null character when encoding.
    tmp.resize(expectedLen + 1);
    int len = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(&tmp[0]),
        reinterpret_cast<const unsigned char*>(binary.data()),
        static_cast<int>(binary.size()));
    if (len >= 0 && static_cast<size_t>(len) == expectedLen) {
        base64String.append(tmp.data());
        return true;
    } else {
        return false;
    }
}

bool decodeBase64(const std::string& base64String, Bytes& binary) noexcept {
    Bytes tmp;
    if (!preprocessBase64(base64String, tmp)) {
        return false;
    }
    if (tmp.empty()) {
        return true;
    }

    size_t expectedLen = tmp.size() / B64CHAR_BLOCK * B64BIN_BLOCK;
    size_t index = binary.size();
    binary.resize(index + expectedLen);
    int len = EVP_DecodeBlock(&binary[index], &tmp[0], static_cast<int>(tmp.size()));

    return len >= 0 && static_cast<size_t>(len) <= expectedLen;
}

}  // namespace codecUtils
}  // namespace alexaClientSDK

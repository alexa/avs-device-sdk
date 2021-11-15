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

// Real implementations
#include <AVSCommon/Utils/Logger/Logger.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <memory>
#include <vector>

#include "acsdkAssetsCommon/Base64Url.h"

using namespace std;

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/// String to identify log entries originating from this file.
static const std::string TAG{"Base64Url"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool encodeBase64(const std::vector<unsigned char>& binary, std::string* string) {
    if (!string) {
        return false;
    } else if (binary.empty()) {
        string->resize(0);
        return true;
    }
    // Base64 creates 4 output bytes for each 3 bytes of input
    int len = binary.size() / 3 * 4;
    // If input size is not dividable by 3, Base64 creates an additional 4 byte block.
    if (binary.size() % 3) {
        len += 4;
    }
    // OpenSSL encoder adds a null character when encoding.
    len++;
    std::vector<char> tmp;
    tmp.resize(len);
    EVP_EncodeBlock((unsigned char*)&tmp[0], (const unsigned char*)binary.data(), binary.size());
    *string = tmp.data();
    return true;
}

bool decodeBase64(const std::string& string, std::vector<unsigned char>* binary) noexcept {
    if (!binary) {
        return false;
    } else if (string.empty()) {
        binary->resize(0);
        return true;
    }
    int len = string.size() / 4 * 3;
    if (string.size() % 4) {
        len += 3;
    }
    int index = 0;
    binary->resize(index + len);
    len = EVP_DecodeBlock(&(*binary)[index], (const unsigned char*)&string[0], string.size());
    if (len >= 0) {
        return true;
    } else {
        return false;
    }
}

bool Base64Url::encode(const string& plain, string& encoded) {
    vector<unsigned char> binaryInput{plain.begin(), plain.end()};
    string dest;
    if (!encodeBase64(binaryInput, &dest)) {
        ACSDK_ERROR(LX("encode").m("Failed to encode string").d("input", plain));
        return false;
    }

    auto curl = unique_ptr<CURL, decltype(&curl_easy_cleanup)>(curl_easy_init(), curl_easy_cleanup);
    if (curl == nullptr) {
        ACSDK_ERROR(LX("encode").m("Can't initialize curl"));
        return false;
    }
    auto escaped = unique_ptr<char, decltype(&curl_free)>(
            curl_easy_escape(curl.get(), dest.data(), static_cast<int>(dest.length())), curl_free);
    if (escaped == nullptr) {
        ACSDK_ERROR(LX("encode").m("Can't URL-escape the string"));
        return false;
    }

    encoded = escaped.get();
    return true;
}

bool Base64Url::decode(const string& encoded, string& plain) {
    auto curl = unique_ptr<CURL, decltype(&curl_easy_cleanup)>(curl_easy_init(), curl_easy_cleanup);
    if (curl == nullptr) {
        ACSDK_ERROR(LX("decode").m("Can't initialize curl"));
        return false;
    }

    if (encoded.size() > INT_MAX) {
        ACSDK_ERROR(LX("decode").m("Unexpected input size").d("size", encoded.size()));
        return false;
    }

    int unescapedSize;
    auto unescaped = unique_ptr<char, decltype(&curl_free)>(
            curl_easy_unescape(curl.get(), encoded.data(), static_cast<int>(encoded.size()), &unescapedSize),
            curl_free);
    if (unescaped == nullptr) {
        ACSDK_ERROR(LX("decode").m("Can't URL-escape the string"));
        return false;
    }

    vector<unsigned char> binaryOutput;
    if (!decodeBase64(unescaped.get(), &binaryOutput)) {
        ACSDK_ERROR(LX("decode").m("Failed to decode string").d("input", encoded));
        return false;
    }
    plain = string{binaryOutput.begin(), binaryOutput.end()};
    return true;
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

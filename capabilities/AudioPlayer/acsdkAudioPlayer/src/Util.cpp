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

#include "acsdkAudioPlayer/Util.h"
#include <AVSCommon/Utils/Logger/Logger.h>

#include <openssl/md5.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayerUtil");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// Number of Bits In Byte.
static const int kBitsInByte(8);
// Number of Bits In Hex.
static const int kBitsInHex(4);
// Number of Hex characters in Byte.
static const int kHexInByte(kBitsInByte / kBitsInHex);

std::string Util::generateMD5Hash(const std::string& key) {
    // mini cache
    static std::string lastKey;
    static std::string lastHash;

    if (lastKey == key) {
        return lastHash;
    }

    uint8_t hashOutput[MD5_DIGEST_LENGTH];
    if (!MD5((uint8_t*)key.data(), key.size(), hashOutput)) {
        ACSDK_ERROR(LX(__FUNCTION__).d("reason", "failedToGenerateHash"));
        return std::string();
    }

    char hashString[MD5_DIGEST_LENGTH * kHexInByte + 1];
    // Generate Hexadecimal string from given hash output.
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(hashString + (i * kHexInByte), kHexInByte + 1, "%02x", hashOutput[i]);
    }
    lastKey = key;
    lastHash = std::string(hashString);
    return lastHash;
}

std::string Util::getDomainNameFromUrl(const std::string& url) {
    // locate scheme seperator
    std::string::size_type scheme_sep = url.find("://");
    if (scheme_sep == std::string::npos) {
        return std::string();
    }
    std::string::size_type begin = scheme_sep + 3;

    // locate end of hostname
    std::string::size_type end = url.find_first_of(":/?", begin);
    if (end == std::string::npos) {
        end = url.length();
    }
    return url.substr(begin, end - begin);
}

}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK
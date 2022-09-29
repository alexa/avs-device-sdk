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

#include <acsdk/CodecUtils/Hex.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayerUtil");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::string Util::generateMD5Hash(
    const std::string& key,
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory) {
    using namespace alexaClientSDK::cryptoInterfaces;
    using namespace alexaClientSDK::codecUtils;

    static std::mutex keyMutex;
    // mini cache
    static std::string lastKey;
    static std::string lastHash;
    {
        std::lock_guard<std::mutex> lock{keyMutex};
        if (lastKey == key) {
            return lastHash;
        }
    }

    if (!cryptoFactory) {
        ACSDK_ERROR(LX("generateMD5Hashfailed").d("reason", "nullcryptoFactory"));
        return "";
    }

    auto digest = cryptoFactory->createDigest(DigestType::MD5);
    if (!digest) {
        ACSDK_ERROR(LX("generateMD5Hashfailed").d("reason", "createDigestFailed"));
        return "";
    }

    if (!digest->processString(key)) {
        ACSDK_ERROR(LX("generateMD5Hashfailed").d("reason", "digestProcessFailed"));
        return "";
    }

    DigestInterface::DataBlock digestData;
    if (!digest->finalize(digestData)) {
        ACSDK_ERROR(LX("generateMD5Hashfailed").d("reason", "digestFinalizeFailed"));
        return "";
    }
    std::string hashString;
    if (!encodeHex(digestData, hashString)) {
        ACSDK_ERROR(LX("generateMD5Hashfailed").d("reason", "encodeHexFailed"));
        return "";
    }
    {
        std::lock_guard<std::mutex> lock{keyMutex};
        lastKey = key;
        lastHash = std::move(hashString);
        return lastHash;
    }
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

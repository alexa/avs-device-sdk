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

#include <algorithm>

#include "AVSCommon/Utils/Logger/Logger.h"
#include <AVSCommon/Utils/MediaPlayer/PlaybackContext.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// String to identify log entries originating from this file.
#define TAG "PlaybackContext"

/**
 * Create a @c LogEntry using this file's @c TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

const std::string PlaybackContext::HTTP_HEADERS = "httpHeaders";
const std::string PlaybackContext::HTTP_KEY_HEADERS = "key";
const std::string PlaybackContext::HTTP_MANIFEST_HEADERS = "manifest";
const std::string PlaybackContext::HTTP_AUDIOSEGMENT_HEADERS = "audioSegment";
const std::string PlaybackContext::HTTP_ALL_HEADERS = "all";
static const std::string AUTHORIZATION = "Authorization";
static const std::string ALLOWED_PREFIX = "x-";
static const std::string ALLOWED_PREFIX_CAP = "X-";
static const std::string COOKIE = "Cookie";
static const unsigned int MIN_KEY_LENGTH = 3;
static const unsigned int MAX_KEY_LENGTH = 256;
static const unsigned int MAX_VALUE_LENGTH = 4096;
static const unsigned int MAX_ENTRIES_PER_CONFIG = 20;

bool validateIfNotMalicious(const std::string& header) {
    return (!header.empty() && header.find("\r") == std::string::npos && header.find("\n") == std::string::npos);
}

/**
 * Helper function to validate the headers.
 *
 * @param[out] headerConfig to be validated and the invalid entries will be deleted.
 * @return <true, true> if valid and not malicious.
 */
static std::pair<bool, bool> validatePlaybackContextHeadersInternal(HeaderConfig* headerConfig) {
    bool foundInvalidHeaders = false;
    bool foundMaliciousHeaders = false;
    for (auto entry = headerConfig->begin(); entry != headerConfig->end();) {
        if (!validateIfNotMalicious(entry->first) || !validateIfNotMalicious(entry->second)) {
            foundMaliciousHeaders = true;
        }
        if (((entry->first.find(ALLOWED_PREFIX) == 0 || entry->first.find(ALLOWED_PREFIX_CAP) == 0) &&
             entry->first.length() >= MIN_KEY_LENGTH && entry->first.length() <= MAX_KEY_LENGTH &&
             entry->second.length() <= MAX_VALUE_LENGTH) ||
            (entry->first.compare(AUTHORIZATION) == 0 && entry->second.length() <= MAX_VALUE_LENGTH) ||
            (entry->first.compare(COOKIE) == 0 && entry->second.length() <= MAX_VALUE_LENGTH)) {
            if (!foundMaliciousHeaders) {
                entry++;
            } else {
                entry = headerConfig->erase(entry);
            }
        } else {
            entry = headerConfig->erase(entry);
            foundInvalidHeaders = true;
        }
    }

    if (foundInvalidHeaders || headerConfig->size() > MAX_ENTRIES_PER_CONFIG) {
        ACSDK_WARN(LX("validateHeadersInternal")
                       .d("found invalid headers:", foundInvalidHeaders)
                       .d("HeadersSize:", headerConfig->size()));
    }

    if (foundMaliciousHeaders) {
        ACSDK_WARN(LX("validateHeadersInternal").d("found malicious headers:", foundMaliciousHeaders));
    }

    // Erase extra headers.
    for (auto entry = headerConfig->begin(); headerConfig->size() > MAX_ENTRIES_PER_CONFIG;) {
        entry = headerConfig->erase(entry);
        foundInvalidHeaders = true;
    }

    return std::make_pair(!foundInvalidHeaders, !foundMaliciousHeaders);
}

std::pair<bool, bool> validatePlaybackContextHeaders(PlaybackContext* playbackContext) {
    std::vector<HeaderConfig*> configs = {&(playbackContext->keyConfig),
                                          &(playbackContext->manifestConfig),
                                          &(playbackContext->audioSegmentConfig),
                                          &(playbackContext->allConfig)};
    std::vector<std::pair<bool, bool>> isHeaderValid(configs.size(), std::make_pair(false, false));

    std::transform(configs.begin(), configs.end(), isHeaderValid.begin(), [](HeaderConfig* config) {
        return validatePlaybackContextHeadersInternal(config);
    });
    return std::make_pair(
        std::all_of(
            isHeaderValid.begin(), isHeaderValid.end(), [](std::pair<bool, bool> isValid) { return isValid.first; }),
        std::all_of(
            isHeaderValid.begin(), isHeaderValid.end(), [](std::pair<bool, bool> isValid) { return isValid.second; }));
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

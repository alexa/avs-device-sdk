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
static const std::string TAG("PlaybackContext");

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
static const unsigned int MIN_KEY_LENGTH = 3;
static const unsigned int MAX_KEY_LENGTH = 256;
static const unsigned int MAX_VALUE_LENGTH = 4096;
static const unsigned int MAX_ENTRIES_PER_CONFIG = 20;

/**
 * Helper function to validate the headers.
 *
 * @param[out] headerConfig to be validated and the invalid entries will be deleted.
 * @return @c true if validation had no errrors, else @c false.
 */
static bool validatePlaybackContextHeadersInternal(HeaderConfig* headerConfig) {
    bool foundInvalidHeaders = false;
    for (auto entry = headerConfig->begin(); entry != headerConfig->end();) {
        if ((entry->first.find(ALLOWED_PREFIX) == 0 && entry->first.length() >= MIN_KEY_LENGTH &&
             entry->first.length() <= MAX_KEY_LENGTH && entry->second.length() <= MAX_VALUE_LENGTH) ||
            (entry->first.compare(AUTHORIZATION) == 0 && entry->second.length() <= MAX_VALUE_LENGTH)) {
            entry++;
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
    // Erase extra headers.
    for (auto entry = headerConfig->begin(); headerConfig->size() > MAX_ENTRIES_PER_CONFIG;) {
        entry = headerConfig->erase(entry);
        foundInvalidHeaders = true;
    }

    return !foundInvalidHeaders;
}

bool validatePlaybackContextHeaders(PlaybackContext* playbackContext) {
    std::vector<HeaderConfig*> configs = {&(playbackContext->keyConfig),
                                          &(playbackContext->manifestConfig),
                                          &(playbackContext->audioSegmentConfig),
                                          &(playbackContext->allConfig)};
    std::vector<bool> isHeaderValid(configs.size(), false);

    std::transform(configs.begin(), configs.end(), isHeaderValid.begin(), [](HeaderConfig* config) {
        return validatePlaybackContextHeadersInternal(config);
    });

    return std::all_of(isHeaderValid.begin(), isHeaderValid.end(), [](bool isValid) { return isValid; });
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

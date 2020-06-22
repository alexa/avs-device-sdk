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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKCONTEXT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKCONTEXT_H_

#include <map>
#include <string>

#include "AVSCommon/Utils/PlatformDefinitions.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// Header Key, Value pairs.
typedef std::map<std::string, std::string> HeaderConfig;

/*
 * An object that contains Playback Context for a source media.
 */
struct PlaybackContext {
    /// Http Headers in play directive
    static const avscommon_EXPORT std::string HTTP_HEADERS;

    /// Key in PlayDirective for the Http Headers that have to be sent with key/license requests
    static const avscommon_EXPORT std::string HTTP_KEY_HEADERS;

    /// Key in PlayDirective for the Http Headers that have to be sent with manifest requests
    static const avscommon_EXPORT std::string HTTP_MANIFEST_HEADERS;

    /// Key in PlayDirective for the Http Headers that have to be sent with audioSegment requests
    static const avscommon_EXPORT std::string HTTP_AUDIOSEGMENT_HEADERS;

    /// Key in PlayDirective for the Http Headers that have to be sent with all requests
    static const avscommon_EXPORT std::string HTTP_ALL_HEADERS;

    /// Headers to be sent while fetching license.
    HeaderConfig keyConfig;

    /// Headers to be sent while fetching manifest.
    HeaderConfig manifestConfig;

    /// Headers to be sent while fetching data segments.
    HeaderConfig audioSegmentConfig;

    /// Headers to be sent for all out going requests.
    HeaderConfig allConfig;
};

/**
 * Validate the headers. Delete the invalid entries
 *
 * @param[out] playbackContext to be validated and the invalid entries will be deleted.
 * @return @c true if validation had no errrors, else @c false.
 */
bool validatePlaybackContextHeaders(PlaybackContext* playbackContext);

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKCONTEXT_H_

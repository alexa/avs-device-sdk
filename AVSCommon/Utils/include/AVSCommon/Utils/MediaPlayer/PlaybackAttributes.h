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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKATTRIBUTES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKATTRIBUTES_H_

#include <chrono>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * Struct to hold playback attributes of a track. This struct is used for
 * both reporting playback options (as part of an adaptive bitrate scheme
 * such as MPEG-DASH) for Premium Audio, and reporting which playback option the device
 * chose.
 */
struct PlaybackAttributes {
    /// Identifier for the stream currently being played from the manifest. For DASH, this will be pulled from a
    /// SupplementalProperty under each representation, with the schemeIdURI="tag:amazon.com,2019:dash:StreamId"
    std::string name;

    /// Encoding of the stream being played. For DASH, this will be pulled from the "representation" tag in an
    /// Adaptation Set.
    std::string codec;

    /// Audio sampling rate of the stream being played in Hertz. For DASH, this will be pulled from the "representation"
    /// tag in an Adaptation Set.
    long samplingRateInHertz;

    /// Bitrate of the stream being played in bits per second. For DASH, this will be pulled from the "representation"
    /// tag in an Adaptation Set.
    long dataRateInBitsPerSecond;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_PLAYBACKATTRIBUTES_H_

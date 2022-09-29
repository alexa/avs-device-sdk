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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIADESCRIPTION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIADESCRIPTION_H_

#include <unordered_map>
#include <vector>

#include <AVSCommon/AVS/PlayBehavior.h>
#include <AVSCommon/SDKInterfaces/Audio/MixingBehavior.h>
#include <AVSCommon/Utils/AudioAnalyzer/AudioAnalyzerState.h>
#include <AVSCommon/Utils/Optional.h>
#include <Captions/CaptionData.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// Play behavior key to be included in the additional data.
static std::string PLAY_BEHAVIOR = "playBehavior";

/*
 * An object that contains all playback related information needed from the media CA.
 */
struct MediaDescription {
    /// Mixing behavior of the stream.
    sdkInterfaces::audio::MixingBehavior mixingBehavior;

    /// Focus channel identifies the content type acquiring focus following FocusManger naming convention.
    std::string focusChannel;  // "Dialog", "Communications", "Alert", "Content", "Visual"

    /// String identifier of the source.
    std::string trackId;

    /// Object that contains CaptionData with unprocessed caption content and metadata of a particular format.
    Optional<captions::CaptionData> caption;

    /// Audio analyzers used to process provided audio content.
    Optional<std::vector<audioAnalyzer::AudioAnalyzerState>> analyzers;

    /// All additional information to be provided, including PlayBehavior.
    std::unordered_map<std::string, std::string> additionalData;

    /// Are all of the required values in the Media Description struct set
    bool enabled;
};

/**
 * Builds an empty Media Description object.
 * @return an empty Media Description object.
 */
inline MediaDescription emptyMediaDescription() {
    return MediaDescription{sdkInterfaces::audio::MixingBehavior(),
                            "",
                            "",
                            Optional<captions::CaptionData>(),
                            Optional<std::vector<audioAnalyzer::AudioAnalyzerState>>(),
                            std::unordered_map<std::string, std::string>(),
                            false};
}

/**
 * Write a @c MediaDescription value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param mediaDescription The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const MediaDescription& mediaDescription) {
    stream << "MixingBehavior:";
    switch (mediaDescription.mixingBehavior) {
        case sdkInterfaces::audio::MixingBehavior::BEHAVIOR_PAUSE:
            stream << "BEHAVIOR_PAUSE";
            break;
        case sdkInterfaces::audio::MixingBehavior::BEHAVIOR_DUCK:
            stream << "BEHAVIOR_DUCK";
            break;
    }
    stream << ", Channel:" << mediaDescription.focusChannel << ", ";
    stream << ", TrackId:" << mediaDescription.trackId;
    if (mediaDescription.caption.hasValue()) {
        stream << ", CaptionData:{format:" << (mediaDescription.caption.value()).format;
        stream << ", content:" << (mediaDescription.caption.value()).content << "}";
    }
    if (mediaDescription.analyzers.hasValue()) {
        stream << ", Analyzers:{";
        const auto analyzersCopy = mediaDescription.analyzers.value();
        for (auto iter = analyzersCopy.begin(); iter != analyzersCopy.end(); iter++) {
            stream << "{name:" << (*iter).name;
            stream << ", enableState:" << (*iter).enableState << "}";
        }
        stream << "}";
    }
    stream << ", AdditionalData:{";
    const auto additionalDataCopy = mediaDescription.additionalData;
    for (auto iter = additionalDataCopy.begin(); iter != additionalDataCopy.end(); iter++) {
        stream << "{" << (*iter).first;
        stream << ":" << (*iter).second << "}";
    }
    stream << "}, enabled: " << (mediaDescription.enabled ? "true" : "false") << " }";
    return stream;
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_MEDIADESCRIPTION_H_

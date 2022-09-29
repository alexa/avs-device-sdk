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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLVIDEOCONFIGURATION_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLVIDEOCONFIGURATION_H_

#include <set>
#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
/**
 * Video-related settings in Alexa.Presentation.APL.Video interface
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl-video.html
 */
struct VideoSettings {
    /// Enum class defining the supported video codecs
    enum class Codec {
        /// H.264 at a maximum resolution of 1080p @ 30fps (codec level 4.1)
        H_264_41,

        /// H.264 at a maximum resolution of 1080p @ 60fps (codec level 4.2)
        H_264_42
    };

    /// A collection of one or more supported codecs by the device.
    std::set<Codec> codecs;

    /**
     * Converts Codec enum to string
     * @param codec Codec enum value
     * @return String representing enum value
     */
    static const std::string codecToString(const Codec codec) {
        switch (codec) {
            case Codec::H_264_41:
                return "H_264_41";
            case Codec::H_264_42:
                return "H_264_42";
        }
        return "";
    }

    /**
     * Converts Codec string to enum value
     * @param codec string
     * @return Optional object with enum value corresponding to the string if found
     */
    static const alexaClientSDK::avsCommon::utils::Optional<Codec> stringToCodec(const std::string codec) {
        alexaClientSDK::avsCommon::utils::Optional<Codec> codecOpt;
        if (codec == "H_264_41") {
            codecOpt.set(VideoSettings::Codec::H_264_41);
        } else if (codec == "H_264_42") {
            codecOpt.set(VideoSettings::Codec::H_264_42);
        }
        return codecOpt;
    }
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLVIDEOCONFIGURATION_H_

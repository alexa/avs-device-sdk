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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_APPLICATIONMEDIAINTERFACES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_APPLICATIONMEDIAINTERFACES_H_

#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Structure used to identify an application's media interfaces that are typically created together.
 */
struct ApplicationMediaInterfaces {
    /**
     * Constructor
     *
     * @param mediaPlayer MediaPlayer
     * @param speaker Speaker
     * @param equalizer Equalizer
     * @param requiresShutdown Components requiring shutdown
     * @param channelVolume Channel volume interface. Optional; default is a nullptr.
     */
    ApplicationMediaInterfaces(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer,
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolume = nullptr);

    /// Media Player
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer;

    /// Speaker implementation
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;

    /// Equalizer implementation
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer;

    /// Requires Shutdown implementation
    std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;

    /// ChannelVolume implementation
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolume;
};

inline ApplicationMediaInterfaces::ApplicationMediaInterfaces(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer,
    std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolume) :
        mediaPlayer{mediaPlayer},
        speaker{speaker},
        equalizer{equalizer},
        requiresShutdown{requiresShutdown},
        channelVolume{channelVolume} {
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_APPLICATIONMEDIAINTERFACES_H_

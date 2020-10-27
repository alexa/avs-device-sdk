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

#ifndef ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_APPLICATIONAUDIOPIPELINEFACTORYINTERFACE_H_
#define ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_APPLICATIONAUDIOPIPELINEFACTORYINTERFACE_H_

#include <string>

#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>

#include "PooledApplicationMediaInterfaces.h"

namespace alexaClientSDK {
namespace acsdkApplicationAudioPipelineFactoryInterfaces {

/**
 * ApplicationAudioPipelineFactoryInterface is a factory interface to create @c ApplicationMediaInterfaces (media
 * player, equalizer, speaker).
 */
class ApplicationAudioPipelineFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ApplicationAudioPipelineFactoryInterface() = default;

    /**
     * Creates a new @c ApplicationMediaInterfaces struct of related application media interfaces.
     *
     * @param name The name of this media player.
     * @param equalizerAvailable Whether an equalizer is available for this media player. If equalizers are enabled
     * in SDK configuration, the equalizer will be added to the EqualizerRuntimeSetup.
     * @param enableLiveMode Whether live mode is enabled for this media player.
     * @param isCaptionable Whether this media player is a source for captions.
     * @param channelVolumeType Optional @c ChannelVolumeType of the speaker. Default is AVS_SPEAKER_VOLUME.
     * @param volumeCurve Optional channel volume curve to be used for channel volume attenuation.
     *
     * @return A new @c ApplicationMediaInterfaces struct, including media player and other interfaces.
     */
    virtual std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces> createApplicationMediaInterfaces(
        const std::string& name,
        bool equalizerAvailable = false,
        bool enableLiveMode = false,
        bool isCaptionable = false,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType =
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        std::function<int8_t(int8_t)> volumeCurve = nullptr) = 0;

    /**
     * Creates a new @c PooledApplicationMediaInterfaces struct of multiple media players and related interfaces.
     *
     * @param name The name of these media players for logging purposes.
     * @param numMediaPlayers The number of media players to create.
     * @param equalizerAvailable Whether an equalizer is available. If equalizers are enabled
     * in SDK configuration, the equalizers will be added to the EqualizerRuntimeSetup.
     * @param enableLiveMode Whether live mode is enabled for these players.
     * @param isCaptionable Whether these players are a source for captions.
     * @param channelVolumeType Optional @c ChannelVolumeType of the speakers. Default is AVS_SPEAKER_VOLUME.
     * @param volumeCurve Optional channel volume curve to be used for channel volume attenuation.
     *
     * @return A new @c PooledApplicationMediaInterfaces struct, including media players and other interfaces.
     */
    virtual std::shared_ptr<PooledApplicationMediaInterfaces> createPooledApplicationMediaInterfaces(
        const std::string& name,
        int numMediaPlayers,
        bool equalizerAvailable = false,
        bool enableLiveMode = false,
        bool isCaptionable = false,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType =
            avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        std::function<int8_t(int8_t)> volumeCurve = nullptr) = 0;
};

}  // namespace acsdkApplicationAudioPipelineFactoryInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_APPLICATIONAUDIOPIPELINEFACTORYINTERFACE_H_

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

#ifndef ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_POOLEDAPPLICATIONMEDIAINTERFACES_H_
#define ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_POOLEDAPPLICATIONMEDIAINTERFACES_H_

#include <unordered_set>

#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>

namespace alexaClientSDK {
namespace acsdkApplicationAudioPipelineFactoryInterfaces {

/**
 * Structure to contain a pool of related application's media interfaces that are typically created together.
 *
 * @note Each set may be different sizes. For example, the pool may contain multiple media players
 * that share a single channelVolumeInterface.
 */
struct PooledApplicationMediaInterfaces {
    /// Media Players
    std::unordered_set<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> mediaPlayers;

    /// Speaker implementations
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> speakers;

    /// Equalizer implementations
    std::unordered_set<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface>> equalizers;

    /// Requires Shutdown implementations
    std::unordered_set<std::shared_ptr<avsCommon::utils::RequiresShutdown>> requiresShutdowns;

    /// ChannelVolume implementations
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> channelVolumes;
};

}  // namespace acsdkApplicationAudioPipelineFactoryInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAPPLICATIONAUDIOPIPELINEFACTORYINTERFACES_POOLEDAPPLICATIONMEDIAINTERFACES_H_

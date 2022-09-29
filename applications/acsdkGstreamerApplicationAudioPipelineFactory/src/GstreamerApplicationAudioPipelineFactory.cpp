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

#include <MediaPlayer/MediaPlayer.h>

#include "acsdkApplicationAudioPipelineFactory/GstreamerApplicationAudioPipelineFactory.h"

namespace alexaClientSDK {
namespace acsdkApplicationAudioPipelineFactory {

using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace acsdkEqualizerInterfaces;
using namespace acsdkShutdownManagerInterfaces;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::mediaPlayer;
using namespace captions;

/// String to identify log entries originating from this file.
#define TAG "GstreamerApplicationAudioPipelineFactory"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
GstreamerApplicationAudioPipelineFactory::createApplicationAudioPipelineFactoryInterface(
    const std::shared_ptr<ChannelVolumeFactoryInterface>& channelVolumeFactory,
    const std::shared_ptr<SpeakerManagerInterface>& speakerManager,
    const std::shared_ptr<EqualizerRuntimeSetupInterface>& equalizerRuntimeSetup,
    const std::shared_ptr<HTTPContentFetcherInterfaceFactoryInterface>& httpContentFetcherFactory,
    const std::shared_ptr<ShutdownNotifierInterface>& shutdownNotifier,
    const std::shared_ptr<CaptionManagerInterface>& captionManager) {
    ACSDK_DEBUG5(LX(__func__));
    if (!channelVolumeFactory || !speakerManager || !equalizerRuntimeSetup || !httpContentFetcherFactory ||
        !shutdownNotifier) {
        ACSDK_ERROR(LX("createFailed")
                        .d("isChannelVolumeFactoryNull", !channelVolumeFactory)
                        .d("isSpeakerManagerNull", !speakerManager)
                        .d("isEqualizerRuntimeSetupNull", !equalizerRuntimeSetup)
                        .d("isHttpContentFetcherFactoryNull", !httpContentFetcherFactory)
                        .d("isShutdownNotifierNull", !shutdownNotifier));
        return nullptr;
    }

    return std::shared_ptr<GstreamerApplicationAudioPipelineFactory>(new GstreamerApplicationAudioPipelineFactory(
        channelVolumeFactory,
        speakerManager,
        equalizerRuntimeSetup,
        httpContentFetcherFactory,
        shutdownNotifier,
        captionManager));
}

std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces> GstreamerApplicationAudioPipelineFactory::
    createApplicationMediaInterfaces(
        const std::string& name,
        bool equalizerAvailable,
        bool enableLiveMode,
        bool isCaptionable,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
        std::function<int8_t(int8_t)> volumeCurve) {
    bool enableEqualizer = equalizerAvailable && m_equalizerRuntimeSetup->isEnabled();

    auto mediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        m_httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!mediaPlayer) {
        ACSDK_ERROR(LX("createApplicationMediaInterfacesFailed").d("name", name));
        return nullptr;
    }
    auto speaker = std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer);
    auto channelVolume = m_channelVolumeFactory->createChannelVolumeInterface(speaker, channelVolumeType, volumeCurve);
    m_speakerManager->addChannelVolumeInterface(channelVolume);

    auto equalizer =
        std::static_pointer_cast<alexaClientSDK::acsdkEqualizerInterfaces::EqualizerInterface>(mediaPlayer);
    m_equalizerRuntimeSetup->addEqualizer(equalizer);

    auto requiresShutdown = std::static_pointer_cast<alexaClientSDK::avsCommon::utils::RequiresShutdown>(mediaPlayer);
    m_shutdownNotifier->addObserver(requiresShutdown);

    if (isCaptionable && m_captionManager) {
        m_captionManager->addMediaPlayer(mediaPlayer);
    }

    auto applicationMediaInterfaces =
        std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, equalizer, requiresShutdown, channelVolume);
    return applicationMediaInterfaces;
}

std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces>
GstreamerApplicationAudioPipelineFactory::createPooledApplicationMediaInterfaces(
    const std::string& name,
    int numMediaPlayers,
    bool equalizerAvailable,
    bool enableLiveMode,
    bool isCaptionable,
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
    std::function<int8_t(int8_t)> volumeCurve) {
    if (numMediaPlayers < 1) {
        ACSDK_ERROR(LX("createPooledApplicationMediaInterfacesFailed")
                        .d("invalid numMediaPlayers", numMediaPlayers)
                        .d("name", name));
        return nullptr;
    }

    auto pool = std::make_shared<acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces>(
        acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces());
    for (int i = 0; i < numMediaPlayers; i++) {
        auto applicationMediaInterfaces = createApplicationMediaInterfaces(
            name, equalizerAvailable, enableLiveMode, isCaptionable, channelVolumeType, volumeCurve);
        if (!applicationMediaInterfaces) {
            ACSDK_ERROR(LX("createPooledApplicationMediaInterfacesFailed")
                            .d("failed to create ApplicationMediaInterfaces", name));
            return nullptr;
        }

        if (!applicationMediaInterfaces->mediaPlayer) {
            ACSDK_ERROR(LX("createPooledApplicationMediaInterfacesFailed")
                            .d("reason", "media player is a nullptr")
                            .d("name", name));
            return nullptr;
        }

        pool->mediaPlayers.insert(applicationMediaInterfaces->mediaPlayer);

        if (applicationMediaInterfaces->speaker) {
            pool->speakers.insert(applicationMediaInterfaces->speaker);
        }

        if (applicationMediaInterfaces->equalizer) {
            pool->equalizers.insert(applicationMediaInterfaces->equalizer);
        }

        if (applicationMediaInterfaces->channelVolume) {
            pool->channelVolumes.insert(applicationMediaInterfaces->channelVolume);
        }

        if (applicationMediaInterfaces->requiresShutdown) {
            pool->requiresShutdowns.insert(applicationMediaInterfaces->requiresShutdown);
        }
    }

    return pool;
}

GstreamerApplicationAudioPipelineFactory::GstreamerApplicationAudioPipelineFactory(
    const std::shared_ptr<ChannelVolumeFactoryInterface>& channelVolumeFactory,
    const std::shared_ptr<SpeakerManagerInterface>& speakerManager,
    const std::shared_ptr<EqualizerRuntimeSetupInterface>& equalizerRuntimeSetup,
    const std::shared_ptr<HTTPContentFetcherInterfaceFactoryInterface>& httpContentFetcherFactory,
    const std::shared_ptr<ShutdownNotifierInterface>& shutdownNotifier,
    const std::shared_ptr<CaptionManagerInterface>& captionManager) :
        m_speakerManager{speakerManager},
        m_channelVolumeFactory{channelVolumeFactory},
        m_httpContentFetcherFactory{httpContentFetcherFactory},
        m_shutdownNotifier{shutdownNotifier},
        m_equalizerRuntimeSetup{equalizerRuntimeSetup},
        m_captionManager{captionManager} {
}

}  // namespace acsdkApplicationAudioPipelineFactory
}  // namespace alexaClientSDK

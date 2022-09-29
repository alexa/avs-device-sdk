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

#include "DefaultClient/StubApplicationAudioPipelineFactory.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::mediaPlayer;

/// String to identify log entries originating from this file.
#define TAG "StubApplicationAudioPipelineFactory"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<StubApplicationAudioPipelineFactory> StubApplicationAudioPipelineFactory::create(
    const std::shared_ptr<ChannelVolumeFactoryInterface>& channelVolumeFactory) {
    if (!channelVolumeFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullChannelVolumeFactory"));
        return nullptr;
    }
    return std::shared_ptr<StubApplicationAudioPipelineFactory>(
        new StubApplicationAudioPipelineFactory(channelVolumeFactory));
}

StubApplicationAudioPipelineFactory::StubApplicationAudioPipelineFactory(
    const std::shared_ptr<ChannelVolumeFactoryInterface>& channelVolumeFactory) :
        m_channelVolumeFactory{channelVolumeFactory} {};

void StubApplicationAudioPipelineFactory::addSpeakerManager(
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>& speakerManager) {
    if (!speakerManager) {
        ACSDK_ERROR(LX("addSpeakerManagerFailed").m("nullSpeakerManager"));
        return;
    }
    m_speakerManager = speakerManager;
}

void StubApplicationAudioPipelineFactory::addCaptionManager(
    std::shared_ptr<captions::CaptionManagerInterface>& captionManager) {
    if (!captionManager) {
        ACSDK_DEBUG5(LX("addCaptionManager").m("captions disabled"));
    }

    m_captionManager = captionManager;
}

bool StubApplicationAudioPipelineFactory::addApplicationMediaInterfaces(
    const std::string& name,
    const std::shared_ptr<MediaPlayerInterface>& mediaPlayer,
    const std::shared_ptr<SpeakerInterface>& speaker) {
    std::lock_guard<std::mutex> lock{m_applicationMediaInterfacesMapMutex};

    auto it = m_applicationMediaInterfacesMap.find(name);
    if (m_applicationMediaInterfacesMap.end() != it) {
        ACSDK_ERROR(LX("addApplicationMediaInterfacesFailed").d("reason", "already exists").d("name", name));
        return false;
    }

    std::vector<std::shared_ptr<ApplicationMediaInterfaces>> interfaces;
    interfaces.emplace_back(std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, nullptr, nullptr));

    std::pair<std::string, std::vector<std::shared_ptr<ApplicationMediaInterfaces>>> pair(name, interfaces);
    m_applicationMediaInterfacesMap.insert(pair);
    return true;
}

bool StubApplicationAudioPipelineFactory::addApplicationMediaInterfaces(
    const std::string& name,
    std::vector<std::pair<
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>> mediaInterfaces) {
    std::lock_guard<std::mutex> lock{m_applicationMediaInterfacesMapMutex};

    auto it = m_applicationMediaInterfacesMap.find(name);
    if (m_applicationMediaInterfacesMap.end() != it) {
        ACSDK_ERROR(LX("addApplicationMediaInterfacesFailed").d("reason", "already exists").d("name", name));
        return false;
    }

    for (auto& mediaInterface : mediaInterfaces) {
        if (!mediaInterface.first || !mediaInterface.second) {
            ACSDK_ERROR(
                LX("addApplicationMediaInterfacesFailed").d("reason", "nullptr in mediaInterfaces").d("name", name));
            return false;
        }
    }

    std::vector<std::shared_ptr<ApplicationMediaInterfaces>> allInterfaces;
    for (auto& mediaInterface : mediaInterfaces) {
        auto interfaces =
            std::make_shared<ApplicationMediaInterfaces>(mediaInterface.first, mediaInterface.second, nullptr, nullptr);
        allInterfaces.emplace_back(interfaces);
    }

    std::pair<std::string, std::vector<std::shared_ptr<ApplicationMediaInterfaces>>> pair(name, allInterfaces);
    m_applicationMediaInterfacesMap.insert(pair);
    return true;
}

std::shared_ptr<ApplicationMediaInterfaces> StubApplicationAudioPipelineFactory::createApplicationMediaInterfaces(
    const std::string& name,
    bool equalizerAvailable,
    bool enableLiveMode,
    bool isCaptionable,
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
    std::function<int8_t(int8_t)> volumeCurve) {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_speakerManager) {
        ACSDK_ERROR(LX("createApplicationMediaInterfacesFailed").d("reason", "null SpeakerManager").d("name", name));
        return nullptr;
    }

    std::lock_guard<std::mutex> lock{m_applicationMediaInterfacesMapMutex};

    auto it = m_applicationMediaInterfacesMap.find(name);

    if (m_applicationMediaInterfacesMap.end() == it) {
        ACSDK_ERROR(LX("createApplicationMediaInterfacesFailed").d("reason", "not found").d("name", name));
        return nullptr;
    }

    auto allInterfaces = it->second;
    if (allInterfaces.empty()) {
        ACSDK_ERROR(LX("createApplicationMediaInterfacesFailed").d("reason", "none available").d("name", name));
        return nullptr;
    }

    auto interfaces = allInterfaces.back();
    auto channelVolume =
        m_channelVolumeFactory->createChannelVolumeInterface(interfaces->speaker, channelVolumeType, volumeCurve);
    m_speakerManager->addChannelVolumeInterface(channelVolume);
    interfaces->channelVolume = channelVolume;

    if (isCaptionable && m_captionManager) {
        m_captionManager->addMediaPlayer(interfaces->mediaPlayer);
    }

    allInterfaces.pop_back();
    return interfaces;
}

std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces>
StubApplicationAudioPipelineFactory::createPooledApplicationMediaInterfaces(
    const std::string& name,
    int numMediaPlayers,
    bool equalizerAvailable,
    bool enableLiveMode,
    bool isCaptionable,
    avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
    std::function<int8_t(int8_t)> volumeCurve) {
    if (numMediaPlayers < 1) {
        ACSDK_ERROR(LX("createPooledApplicationMediaInterfacesFailed")
                        .d("numMediaPlayers less than 0", numMediaPlayers)
                        .d("name", name));
        return nullptr;
    }

    auto pool = std::make_shared<acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces>();
    for (int i = 0; i < numMediaPlayers; i++) {
        auto applicationMediaInterfaces = createApplicationMediaInterfaces(
            name, equalizerAvailable, enableLiveMode, isCaptionable, channelVolumeType, volumeCurve);
        if (!applicationMediaInterfaces) {
            ACSDK_ERROR(
                LX("createPooledApplicationMediaInterfacesFailed").d("ApplicationMediaInterfaces not available", name));
            return nullptr;
        }

        if (!applicationMediaInterfaces->mediaPlayer) {
            ACSDK_ERROR(LX("createPooledApplicationMediaInterfacesFailed")
                            .d("ApplicationMediaInterfaces does not contain media player", name));
            return nullptr;
        }
        pool->mediaPlayers.insert(applicationMediaInterfaces->mediaPlayer);

        if (applicationMediaInterfaces->speaker) {
            pool->speakers.insert(applicationMediaInterfaces->speaker);
        }

        if (applicationMediaInterfaces->channelVolume) {
            pool->channelVolumes.insert(applicationMediaInterfaces->channelVolume);
        }
    }

    return pool;
}

}  // namespace defaultClient
}  // namespace alexaClientSDK

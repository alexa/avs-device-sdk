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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_STUBAPPLICATIONAUDIOPIPELINEFACTORY_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_STUBAPPLICATIONAUDIOPIPELINEFACTORY_H_

#include <memory>
#include <queue>
#include <string>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <Captions/CaptionManagerInterface.h>

namespace alexaClientSDK {
namespace defaultClient {

/**
 * This is a factory class that can be used during the transition to using the manufactory for creating
 * media players and related interfaces. Pre-built media players and speakers can be added to this factory, and the
 * factory will return the pre-built @c ApplicationMediaInterfaces with the specified name when
 * createApplicationMediaInterfaces(...) is called.
 *
 * Other factories such as acsdkGstreamerAudioPipelineFactory should be preferred.
 *
 * This factory will register the pre-built speakers with SpeakerManager, but unlike the real factory implementations,
 * it will not register equalizers with EqualizerRuntimeSetup nor media players with the CaptionManager. Applications
 * that use this stub factory are responsible for doing so outside of this factory.
 *
 */
class StubApplicationAudioPipelineFactory
        : public acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface {
public:
    /**
     * Creates a new @c StubApplicationAudioPipelineFactory.
     *
     * @param channelVolumeFactory The @c ChannelVolumeFactoryInterface to use for creating channel volume interfaces.
     * @return A new @c StubApplicationAudioPipelineFactory.
     */
    static std::shared_ptr<StubApplicationAudioPipelineFactory> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>& channelVolumeFactory);

    /**
     * Adds the @c SpeakerManagerInterface for registering speakers.
     *
     * @param speakerManager The @c SpeakerManagerInterface with which to register speakers.
     */
    void addSpeakerManager(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>& speakerManager);

    /**
     * Adds the @c CaptionManagerInterface for adding captionable media players.
     *
     * @param captionManager The @c CaptionManagerInterface with which to register speakers.
     */
    void addCaptionManager(std::shared_ptr<captions::CaptionManagerInterface>& captionManager);

    /**
     * Adds application media interfaces to the factory for later retrieval when createApplicationMediaInterfaces
     * is called.
     *
     * @param name The name of the media player. This name is used as the key to retrieve the correct application media
     * interfaces when createApplicationMediaInterfaces is called.
     * @param mediaPlayer The @c MediaPlayerInterface to add.
     * @param speaker The @c SpeakerInterface to add.
     */
    bool addApplicationMediaInterfaces(
        const std::string& name,
        const std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>& mediaPlayer,
        const std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>& speaker);

    /**
     * Adds a vector of application media interfaces to the factory for later retrieval when
     * createApplicationMediaInterfaces is called.
     *
     * @param name The name of the media players. This name is used as the key to retrieve the correct application media
     * interfaces when createApplicationMediaInterfaces is called.
     * @param mediaInterfaces A vector of media player/speaker pairs to add.
     */
    bool addApplicationMediaInterfaces(
        const std::string& name,
        std::vector<std::pair<
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>,
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>> mediaInterfaces);

    /// @name ApplicationAudioPipelineFactoryInterface
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces> createApplicationMediaInterfaces(
        const std::string& name,
        bool equalizerAvailable,
        bool enableLiveMode,
        bool isCaptionable,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
        std::function<int8_t(int8_t)> volumeCurve) override;
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::PooledApplicationMediaInterfaces>
    createPooledApplicationMediaInterfaces(
        const std::string& name,
        int numMediaPlayers,
        bool equalizerAvailable,
        bool enableLiveMode,
        bool isCaptionable,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type channelVolumeType,
        std::function<int8_t(int8_t)> volumeCurve) override;
    /// @}

private:
    /**
     * Constructor.
     */
    StubApplicationAudioPipelineFactory(
        const std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>& channelVolumeFactory);

    /// Mutex to synchronize access to m_applicationMediaInterfacesMap.
    std::mutex m_applicationMediaInterfacesMapMutex;

    /**
     * Map of name to vector of @c ApplicationMediaInterfaces. The factory will pop an @c ApplicationMediaInterfaces
     * off the corresponding queue when createApplicationMediaInterfaces is called with the name.
     */
    std::unordered_map<std::string, std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>>>
        m_applicationMediaInterfacesMap;

    /// The @c SpeakerManagerInterface with which to register speakers.
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> m_speakerManager;

    /// The @c CaptionManagerInterface with which to register captionable media players.
    std::shared_ptr<captions::CaptionManagerInterface> m_captionManager;

    /// The @c ChannelVolumeFactoryInterface with which to create channel volume interfaces.
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> m_channelVolumeFactory;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_STUBAPPLICATIONAUDIOPIPELINEFACTORY_H_

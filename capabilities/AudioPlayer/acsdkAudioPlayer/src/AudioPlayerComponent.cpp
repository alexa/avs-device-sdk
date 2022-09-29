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

#include <acsdkManufactory/ComponentAccumulator.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaResourceProvider.h>

#include "acsdkAudioPlayer/AudioPlayer.h"
#include "acsdkAudioPlayer/AudioPlayerComponent.h"

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

using namespace acsdkManufactory;

using DefaultEndpointAnnotation = avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation;

/// String to identify log entries originating from this file.
static const std::string TAG("AudioPlayerComponent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The default number of MediaPlayers used by AudioPlayer CA/
/// Can be overridden in the Configuration using @c AUDIO_MEDIAPLAYER_POOL_SIZE_KEY
static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2;

/// Key for the Audio MediaPlayer pool size.
static const std::string AUDIOPLAYER_CONFIG_KEY("audioPlayer");

/// Key for the Audio MediaPlayer pool size.
static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");

/// Name for the audio media players, used for logging purposes.
static const std::string AUDIO_MEDIA_PLAYER_NAME("AudioMediaPlayer");

std::function<std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>&,
    const std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>&)>
getCreatePooledMediaResourceProviderInterface(const std::string& parentConfigKey) {
    return [parentConfigKey](
               const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& config,
               const std::shared_ptr<
                   acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>& factory)
               -> std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> {
        if (!config || !factory) {
            ACSDK_ERROR(LX("getCreatePooledMediaResourceProviderInterfaceFailed")
                            .d("isConfigurationNodeNull", !config)
                            .d("isApplicationAudioPipelineFactoryNull", !factory));
            return nullptr;
        }

        avsCommon::utils::configuration::ConfigurationNode audioPlayerConfig;
        if (!parentConfigKey.empty()) {
            audioPlayerConfig = (*config)[parentConfigKey];
        } else {
            audioPlayerConfig = (*config)[AUDIOPLAYER_CONFIG_KEY];
        }

        int poolSize;
        audioPlayerConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);

        auto pool =
            factory->createPooledApplicationMediaInterfaces(AUDIO_MEDIA_PLAYER_NAME, poolSize, true, false, true);
        if (!pool) {
            ACSDK_ERROR(LX("getCreatePooledMediaResourceProviderInterfaceFailed")
                            .m("failed to create PooledApplicationMediaInterfaces"));
            return nullptr;
        }

        std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> audioMediaPlayerPool(
            pool->mediaPlayers.begin(), pool->mediaPlayers.end());
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> channelVolumePool(
            pool->channelVolumes.begin(), pool->channelVolumes.end());
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> speakerPool(
            pool->speakers.begin(), pool->speakers.end());
        std::vector<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface>> equalizerPool(
            pool->equalizers.begin(), pool->equalizers.end());

        avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::Fingerprint> fingerprint =
            (*(audioMediaPlayerPool.begin()))->getFingerprint();
        std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> audioResourceProvider;
        if (fingerprint.hasValue()) {
            audioResourceProvider =
                avsCommon::utils::mediaPlayer::PooledMediaResourceProvider::createPooledMediaResourceProviderInterface(
                    audioMediaPlayerPool, channelVolumePool, fingerprint.value());
        } else {
            audioResourceProvider =
                avsCommon::utils::mediaPlayer::PooledMediaResourceProvider::createPooledMediaResourceProviderInterface(
                    audioMediaPlayerPool, channelVolumePool, {});
        }

        if (!audioResourceProvider) {
            ACSDK_ERROR(LX("getCreatePooledMediaResourceProviderInterfaceFailed")
                            .m("failed to create PooledMediaResourceProvider"));
            return nullptr;
        }

        return audioResourceProvider;
    };
}

Component<
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>,
    Import<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>,
    Import<std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>>,
    Import<Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    Import<std::shared_ptr<captions::CaptionManagerInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    Import<Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    Import<std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>>>
getComponent(const std::string& configParentKey) {
    return ComponentAccumulator<>()
        .addRetainedFactory(getCreatePooledMediaResourceProviderInterface(configParentKey))
        .addRequiredFactory(AudioPlayer::createAudioPlayerInterface);
}

Component<
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    Import<std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>>,
    Import<Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>>,
    Import<std::shared_ptr<captions::CaptionManagerInterface>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    Import<Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    Import<std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>>>
getBackwardsCompatibleComponent() {
    return ComponentAccumulator<>().addRequiredFactory(AudioPlayer::createAudioPlayerInterface);
}

}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK

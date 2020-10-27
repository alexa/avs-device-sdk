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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIARESOURCEPROVIDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIARESOURCEPROVIDER_H_

#include <vector>

#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerFactoryInterface.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaResourceProviderInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/**
 * Adapts the legacy @c MediaPlayerFactoryInterface (which provides only media players) to the @c
 * PooledMediaResourceProviderInterface (which can also provide other associated media resources with the media
 * players).
 *
 */
class PooledMediaResourceProvider : public avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface {
public:
    /**
     * Adapts a legacy @c MediaPlayerFactoryInterface to a @c PooledMediaResourceProviderInterface, given the
     * speakers associated with the players managed by the @c MediaPlayerFactoryInterface.
     *
     * @param mediaPlayerFactory The factory to adapt.
     * @param speakers The @c ChannelVolumeInterfaces that the @c PooledMediaResourceProvider will return on
     * getSpeakers().
     * @return A new @c PooledMediaResourceProviderInterface.
     */
    static std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>
    adaptMediaPlayerFactoryInterface(
        std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> mediaPlayerFactory,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> speakers);

    /**
     * Factory method that creates a @c PooledMediaResourceProviderInterface.
     *
     * @param mediaPlayers The pooled media players to manage.
     * @param speakers The @c ChannelVolumeInterfaces that the @c PooledMediaResourceProvider will return on
     * getSpeakers().
     * @param fingerprint Optional fingerprint argument to send to AVS.
     * @return A new @c PooledMediaResourceProviderInterface.
     */
    static std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>
    createPooledMediaResourceProviderInterface(
        std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> mediaPlayers,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> speakers,
        const avsCommon::utils::mediaPlayer::Fingerprint& fingerprint = {});

    ~PooledMediaResourceProvider() override;

    /// @name PooledMediaResourceProviderInterface methods.
    ///@{
    avsCommon::utils::mediaPlayer::Fingerprint getFingerprint() override;
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> acquireMediaPlayer() override;
    bool releaseMediaPlayer(std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer) override;
    bool isMediaPlayerAvailable() override;
    void addObserver(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface> observer) override;
    void removeObserver(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface> observer) override;
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> getSpeakers() const override;
    ///@}

private:
    /**
     * Constructor used to create a PooledMediaResourceProvider from a @c MediaPlayerFactoryInterface and a vector
     * of speakers.
     *
     * @param mediaPlayerFactory The factory to adapt.
     * @param speakers The @c ChannelVolumeInterfaces that the @c PooledMediaResourceProvider will return on
     * getSpeakers().
     */
    PooledMediaResourceProvider(
        std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> mediaPlayerFactory,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> speakers);

    /// The factory being adapted by this instance.
    std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> m_factory;

    /// The collection of associated @c ChannelVolumeInterfaces.
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> m_speakers;
};

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIARESOURCEPROVIDER_H_

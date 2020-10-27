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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIAPLAYERFACTORY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIAPLAYERFACTORY_H_

#include <vector>
#include <unordered_set>

#include <AVSCommon/Utils/MediaPlayer/PooledMediaResourceProviderInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * Implementation of @c MediaPlayerFactoryInterface that manages a pool of pre-created
 * instances of @c MediaPlayerInterface
 */
class PooledMediaPlayerFactory : public avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface {
public:
    /**
     * Create a @c PooledMediaPlayerFactory from a pre-created set of Players.
     *
     * @deprecated Use adaptMediaPlayerFactoryInterface.
     * @param pool pre-created collection of MediaPlayers. Ownership is not transferred.
     */
    static std::unique_ptr<PooledMediaPlayerFactory> create(
        const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& pool,
        const avsCommon::utils::mediaPlayer::Fingerprint& fingerprint = {});

    virtual ~PooledMediaPlayerFactory();

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
    ///@}

protected:
    /**
     * Constructor used to create a PooledMediaPlayerFactory from a pre-created set of players and their associated
     * speakers, channel volumes, and equalizers.
     *
     * @param mediaPlayerPool Pre-created collection of MediaPlayers.
     * @param fingerprint Optional fingerprint argument to send to AVS.
     */
    PooledMediaPlayerFactory(
        const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayerPool,
        const avsCommon::utils::mediaPlayer::Fingerprint& fingerprint = {});

    /**
     * Synchronously notify all observers that a player is available
     */
    void notifyObservers();

    /// The collection of available players
    std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> m_availablePlayerPool;
    /// The collection of players in use
    std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> m_inUsePlayerPool;
    /// The collection of @c ChannelVolumeInterfaces associated with the media players managed by this factory..
    /// Factory observers
    std::unordered_set<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface>> m_observers;
    /// MediaPlayer version information
    avsCommon::utils::mediaPlayer::Fingerprint m_fingerprint;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_POOLEDMEDIAPLAYERFACTORY_H_

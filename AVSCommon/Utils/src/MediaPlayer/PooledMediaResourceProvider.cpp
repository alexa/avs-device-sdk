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

#include <vector>

#include "AVSCommon/Utils/MediaPlayer/PooledMediaPlayerFactory.h"
#include "AVSCommon/Utils/MediaPlayer/PooledMediaResourceProvider.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

using namespace avsCommon::sdkInterfaces;

#define TAG "PooledMediaResourceProvider"

#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> PooledMediaResourceProvider::
    adaptMediaPlayerFactoryInterface(
        std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> mediaPlayerFactory,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> speakers) {
    if (!mediaPlayerFactory) {
        ACSDK_ERROR(LX("adaptMediaPlayerFactoryInterfaceFailed").m("invalid mediaPlayerFactory"));
        return nullptr;
    }
    return std::shared_ptr<PooledMediaResourceProvider>(
        new PooledMediaResourceProvider(std::move(mediaPlayerFactory), speakers));
}

std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface> PooledMediaResourceProvider::
    createPooledMediaResourceProviderInterface(
        std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> mediaPlayers,
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> speakers,
        const avsCommon::utils::mediaPlayer::Fingerprint& fingerprint) {
    auto mediaPlayerFactory = alexaClientSDK::mediaPlayer::PooledMediaPlayerFactory::create(mediaPlayers, fingerprint);
    if (!mediaPlayerFactory) {
        ACSDK_ERROR(LX("createPooledMediaResourceProviderInterfaceFailed").m("invalid mediaPlayerFactory"));
        return nullptr;
    }

    if (speakers.empty()) {
        ACSDK_ERROR(LX("createPooledMediaResourceProviderInterfaceFailed").m("empty speakers"));
        return nullptr;
    }
    for (const auto& speaker : speakers) {
        if (!speaker) {
            ACSDK_ERROR(LX("createPooledMediaResourceProviderInterfaceFailed").m("nullptr in speakers"));
            return nullptr;
        }
    }

    return std::shared_ptr<PooledMediaResourceProvider>(
        new PooledMediaResourceProvider(std::move(mediaPlayerFactory), speakers));
}

PooledMediaResourceProvider::~PooledMediaResourceProvider() {
    m_factory.reset();
    m_speakers.clear();
}

avsCommon::utils::mediaPlayer::Fingerprint PooledMediaResourceProvider::getFingerprint() {
    return m_factory->getFingerprint();
}

std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> PooledMediaResourceProvider::acquireMediaPlayer() {
    return m_factory->acquireMediaPlayer();
}

bool PooledMediaResourceProvider::releaseMediaPlayer(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer) {
    return m_factory->releaseMediaPlayer(mediaPlayer);
}

bool PooledMediaResourceProvider::isMediaPlayerAvailable() {
    return m_factory->isMediaPlayerAvailable();
}

void PooledMediaResourceProvider::addObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface> observer) {
    m_factory->addObserver(observer);
}

void PooledMediaResourceProvider::removeObserver(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryObserverInterface> observer) {
    m_factory->removeObserver(observer);
}

std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> PooledMediaResourceProvider::
    getSpeakers() const {
    return m_speakers;
}

PooledMediaResourceProvider::PooledMediaResourceProvider(
    std::unique_ptr<avsCommon::utils::mediaPlayer::MediaPlayerFactoryInterface> mediaPlayerFactory,
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>> speakers) :
        m_factory{std::move(mediaPlayerFactory)},
        m_speakers(speakers) {
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

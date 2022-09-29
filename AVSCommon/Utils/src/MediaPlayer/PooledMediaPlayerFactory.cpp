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

#include <algorithm>

#include <AVSCommon/Utils/MediaPlayer/PooledMediaPlayerFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils::mediaPlayer;

#define TAG "PooledMediaPlayerFactory"

#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<PooledMediaPlayerFactory> PooledMediaPlayerFactory::create(
    const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& pool,
    const avsCommon::utils::mediaPlayer::Fingerprint& fingerprint) {
    if (pool.size() == 0) {
        ACSDK_ERROR(LX(__func__).m("createFailed").d("reason", "empty Pool"));
        return nullptr;
    }
    for (auto iter = pool.begin(); iter != pool.end(); ++iter) {
        if (*iter == nullptr) {
            ACSDK_ERROR(LX(__func__).m("createFailed").d("reason", "Pool contains nullptr"));
            return nullptr;
        }
    }
    return std::unique_ptr<PooledMediaPlayerFactory>(new PooledMediaPlayerFactory(pool, fingerprint));
}

PooledMediaPlayerFactory::PooledMediaPlayerFactory(
    const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayerPool,
    const Fingerprint& fingerprint) :
        // Parenthesis are used for initializing @c m_fingerprint to work-around a bug in the C++ specification.  see:
        // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1288
        m_fingerprint(fingerprint) {
    m_availablePlayerPool.insert(m_availablePlayerPool.end(), mediaPlayerPool.begin(), mediaPlayerPool.end());
}

PooledMediaPlayerFactory::~PooledMediaPlayerFactory() {
    ACSDK_DEBUG9(LX(__func__));

    m_availablePlayerPool.clear();
    m_inUsePlayerPool.clear();
    m_observers.clear();
}

Fingerprint PooledMediaPlayerFactory::getFingerprint() {
    return m_fingerprint;
}

std::shared_ptr<MediaPlayerInterface> PooledMediaPlayerFactory::acquireMediaPlayer() {
    ACSDK_DEBUG9(LX(__func__));
    if (isMediaPlayerAvailable()) {
        auto pos = m_availablePlayerPool.begin();
        auto player = *pos;
        m_availablePlayerPool.erase(pos);
        m_inUsePlayerPool.push_back(player);

        return player;
    }

    return nullptr;
}

bool PooledMediaPlayerFactory::releaseMediaPlayer(std::shared_ptr<MediaPlayerInterface> mediaPlayer) {
    ACSDK_DEBUG9(LX(__func__));
    if (!mediaPlayer) {
        ACSDK_ERROR(LX(__func__).m("releaseMediaPlayerFailed").d("reason", "null player"));
        return false;
    }

    auto iter = std::find(std::begin(m_inUsePlayerPool), std::end(m_inUsePlayerPool), mediaPlayer);
    if (iter == std::end(m_inUsePlayerPool)) {
        ACSDK_ERROR(LX(__func__).m("releaseMediaPlayerFailed").d("reason", "unknown player"));
        return false;
    }

    m_availablePlayerPool.push_back(mediaPlayer);
    m_inUsePlayerPool.erase(iter);

    if (m_availablePlayerPool.size() == 1) {
        notifyObservers();
    }

    return true;
}

bool PooledMediaPlayerFactory::isMediaPlayerAvailable() {
    return !m_availablePlayerPool.empty();
}

void PooledMediaPlayerFactory::addObserver(std::shared_ptr<MediaPlayerFactoryObserverInterface> observer) {
    m_observers.insert(observer);
}

void PooledMediaPlayerFactory::removeObserver(std::shared_ptr<MediaPlayerFactoryObserverInterface> observer) {
    m_observers.erase(observer);
}

void PooledMediaPlayerFactory::notifyObservers() {
    ACSDK_DEBUG9(LX(__func__));
    for (const auto& observer : m_observers) {
        observer->onReadyToProvideNextPlayer();
    }
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

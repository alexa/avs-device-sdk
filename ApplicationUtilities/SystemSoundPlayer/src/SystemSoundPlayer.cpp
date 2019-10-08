/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/Logger/Logger.h"
#include "SystemSoundPlayer/SystemSoundPlayer.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace systemSoundPlayer {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("SystemSoundPlayer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Utility function to quickly return a ready future with value false.
static std::shared_future<bool> getFalseFuture() {
    auto errPromise = std::promise<bool>();
    errPromise.set_value(false);
    return errPromise.get_future();
}

std::shared_ptr<SystemSoundPlayer> SystemSoundPlayer::create(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> soundPlayerAudioFactory) {
    if (!mediaPlayer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMediaPlayer"));
        return nullptr;
    }
    if (!soundPlayerAudioFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSoundPlayerAudioFactory"));
        return nullptr;
    }

    auto systemSoundPlayer =
        std::shared_ptr<SystemSoundPlayer>(new SystemSoundPlayer(mediaPlayer, soundPlayerAudioFactory));
    mediaPlayer->setObserver(systemSoundPlayer);

    return systemSoundPlayer;
}

std::shared_future<bool> SystemSoundPlayer::playTone(Tone tone) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_sharedFuture.valid()) {
        ACSDK_ERROR(LX("playToneFailed").d("reason", "Already Playing a Tone"));
        return getFalseFuture();
    }

    m_sourceId = avsCommon::utils::mediaPlayer::MediaPlayerInterface::ERROR;
    switch (tone) {
        case Tone::WAKEWORD_NOTIFICATION:
            m_sourceId = m_mediaPlayer->setSource(m_soundPlayerAudioFactory->wakeWordNotificationTone()(), false);
            break;
        case Tone::END_SPEECH:
            m_sourceId = m_mediaPlayer->setSource(m_soundPlayerAudioFactory->endSpeechTone()(), false);
            break;
    }

    if (avsCommon::utils::mediaPlayer::MediaPlayerInterface::ERROR == m_sourceId) {
        ACSDK_ERROR(LX("playToneFailed").d("reason", "setSourceFailed").d("type", "attachment"));
        return getFalseFuture();
    }

    if (!m_mediaPlayer->play(m_sourceId)) {
        ACSDK_ERROR(LX("playToneFailed").d("reason", "playSourceFailed"));
        return getFalseFuture();
    }

    m_sharedFuture = m_playTonePromise.get_future();
    return m_sharedFuture;
}

void SystemSoundPlayer::onPlaybackStarted(SourceId id) {
    ACSDK_DEBUG5(LX(__func__).d("SourceId", id));
}

void SystemSoundPlayer::onPlaybackFinished(SourceId id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ACSDK_DEBUG5(LX(__func__).d("SourceId", id));
    if (m_sourceId != id) {
        ACSDK_ERROR(LX(__func__).d("SourceId", id).d("reason", "sourceId doesn't match played file"));
    }

    finishPlayTone(true);
}

void SystemSoundPlayer::onPlaybackError(
    SourceId id,
    const avsCommon::utils::mediaPlayer::ErrorType& type,
    std::string error) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ACSDK_ERROR(LX(__func__).d("SourceId", id).d("error", error));
    if (m_sourceId != id) {
        ACSDK_ERROR(LX("UnexpectedSourceId")
                        .d("expectedSourceId", m_sourceId)
                        .d("reason", "sourceId doesn't match played file"));
    }

    finishPlayTone(false);
}

void SystemSoundPlayer::finishPlayTone(bool result) {
    m_playTonePromise.set_value(result);

    m_playTonePromise = std::promise<bool>();
    m_sharedFuture = std::shared_future<bool>();
}

SystemSoundPlayer::SystemSoundPlayer(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> soundPlayerAudioFactory) :
        m_mediaPlayer{mediaPlayer},
        m_soundPlayerAudioFactory{soundPlayerAudioFactory} {
}

}  // namespace systemSoundPlayer
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
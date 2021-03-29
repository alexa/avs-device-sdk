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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_SYSTEMSOUNDPLAYER_INCLUDE_SYSTEMSOUNDPLAYER_SYSTEMSOUNDPLAYER_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_SYSTEMSOUNDPLAYER_INCLUDE_SYSTEMSOUNDPLAYER_SYSTEMSOUNDPLAYER_H_

#include <future>
#include <memory>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/SystemSoundAudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/SystemSoundPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/MediaType.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace systemSoundPlayer {

/// String to identify the system sound media player to render system sounds.
static const constexpr char* SYSTEM_SOUND_MEDIA_PLAYER_NAME = "SystemSoundMediaPlayer";

/**
 * This class implements the @c SystemSoundPlayerInterface. This class is responsible for playing the system sounds that
 * Alexa devices make.
 */
class SystemSoundPlayer
        : public avsCommon::sdkInterfaces::SystemSoundPlayerInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface {
public:
    /**
     * Creates a new @c SystemSoundPlayerInterface instance.
     *
     * @param audioPipelineFactory The audio pipeline factory to create the media player and related interfaces.
     * @param audioFactory The audio factory that produces the system sound streams.
     *
     * @return A @c std::shared_ptr to the new @c SystemSoundPlayer instance or nullptr if invalid arguments.
     */
    static std::shared_ptr<SystemSoundPlayerInterface> createSystemSoundPlayerInterface(
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
            audioPipelineFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory);

    /**
     * Creates a new @c SystemSoundPlayer instance.
     *
     * @deprecated Use createSystemSoundPlayerInterface.
     * @param mediaPlayer The mediaPlayer that will play the system sound audio streams.
     * @param soundPlayerAudioFactory The audio factory that produces the system sound streams
     *
     * @return A @c std::shared_ptr to the new @c SystemSoundPlayer instance or nullptr if invalid arguments.
     */
    static std::shared_ptr<SystemSoundPlayer> create(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> soundPlayerAudioFactory);

    /// @name SystemSoundPlayerInterface Functions
    /// @{
    std::shared_future<bool> playTone(Tone tone) override;
    /// @}

    /// @name MediaPlayerObserverInterface Functions
    /// @{
    void onPlaybackFinished(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackError(
        SourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStarted(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onFirstByteRead(SourceId id, const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    /// @}

private:
    /**
     * Constructor
     *
     * @param mediaPlayer The mediaPlayer that will play the system sound audio streams.
     * @param soundPlayerAudioFactory The audio factory that produces the system sound streams
     */
    SystemSoundPlayer(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> soundPlayerAudioFactory);

    /**
     * Helper method to reset class after playback completes
     */
    void finishPlayTone(bool result);

    /// The @c MediaPlayerInterface used to play the audio streams produced by @c
    /// SystemSoundPlayerAudioFactoryInterface.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;
    /// The @c SystemSoundAudioFactoryInterface used to produce the audio streams that make the system sounds.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::SystemSoundAudioFactoryInterface> m_soundPlayerAudioFactory;
    /// The future of the playTonePromise
    std::shared_future<bool> m_sharedFuture;
    /// The promise used to indicate playback of system sounds has finished
    std::promise<bool> m_playTonePromise;
    /// Mutex used to lock m_mediaPlayer during playTone
    std::mutex m_mutex;
    /// The SourceId of the sound file being played
    SourceId m_sourceId;
};

}  // namespace systemSoundPlayer
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_SYSTEMSOUNDPLAYER_INCLUDE_SYSTEMSOUNDPLAYER_SYSTEMSOUNDPLAYER_H_

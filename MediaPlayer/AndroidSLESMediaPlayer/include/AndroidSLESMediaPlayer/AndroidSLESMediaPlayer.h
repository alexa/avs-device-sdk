/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_H_

#include <vector>

#include <SLES/OpenSLES.h>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerInterface.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/PlaylistParser/IterativePlaylistParserInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AndroidUtilities/AndroidSLESEngine.h>
#include <AndroidUtilities/AndroidSLESObject.h>
#include <EqualizerImplementations/EqualizerBandMapperInterface.h>
#include <PlaylistParser/UrlContentToAttachmentConverter.h>

#include "AndroidSLESMediaPlayer/AndroidSLESMediaQueue.h"
#include "AndroidSLESMediaPlayer/FFmpegDecoder.h"
#include "AndroidSLESMediaPlayer/PlaybackConfiguration.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * This class implements an android compatible media player.
 *
 * The implementation uses Android OpenSL ES to play the audio and FFmpeg to decode and resample the media input.
 */
class AndroidSLESMediaPlayer
        : public avsCommon::utils::mediaPlayer::MediaPlayerInterface
        , public avsCommon::sdkInterfaces::audio::EqualizerInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Create AndroidSLESMediaPlayer.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param engine The OpenSL ES engine that is used to access the OpenSL ES media player and output mixer.
     * @param type The type used to categorize the media player speaker for volume control.
     * @param config The playback configuration.
     * @param name The instance name used for logging purpose.
     * @return An instance of the @c AndroidSLESMediaPlayer if successful else @c nullptr.
     */
    static std::unique_ptr<AndroidSLESMediaPlayer> create(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        bool enableEqualizer,
        const PlaybackConfiguration& config = PlaybackConfiguration(),
        const std::string& name = "AndroidMediaPlayer");

    /// @name EqualizerInterface methods.
    ///@{
    void setEqualizerBandLevels(avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap bandLevelMap) override;

    int getMinimumBandLevel() override;

    int getMaximumBandLevel() override;
    ///}@

    /// @name MediaPlayerInterface methods.
    ///@{
    SourceId setSource(
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader,
        const avsCommon::utils::AudioFormat* format) override;
    SourceId setSource(const std::string& url, std::chrono::milliseconds offset, bool repeat) override;
    SourceId setSource(std::shared_ptr<std::istream> stream, bool repeat) override;
    bool play(SourceId id) override;
    bool stop(SourceId id) override;
    bool pause(SourceId id) override;
    bool resume(SourceId id) override;
    std::chrono::milliseconds getOffset(SourceId id) override;
    uint64_t getNumBytesBuffered() override;
    void setObserver(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver) override;
    ///@}

    /**
     * Callback method called by the AndroidSL ES media player when the prefetch status changes.
     *
     * @param event Event that has occurred.
     */
    void onPrefetchStatusChange(SLuint32 event);

    /**
     * Destructor. Stops the media player and shutdown all its members.
     */
    ~AndroidSLESMediaPlayer();

    /**
     * Get the speaker object that can be used to control the media player instance volume.
     *
     * @return An instance of the speaker if successful else @c nullptr.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> getSpeaker();

protected:
    /// @name RequiresShutdown methods.
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param speaker The speaker instance that can be used to control this media player volume.
     * @param engine The OpenSL ES engine that is used to access the OpenSL ES media player and output mixer.
     * @param outputMixObject The output mix object used to control the media player volume.
     * @param playerObject The media player object used to play audio.
     * @param equalizer The equalizer object used to control the media player volume. Could be nullptr if equalizer is
     * not required for this instance.
     * @param playerInterface The interface object used to access the media player functions of the player object.
     * @param config The playback configuration used to setup the android media player.
     * @param name String used to identify the @c AndroidSLESMediaPlayer instance.
     */
    AndroidSLESMediaPlayer(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> engine,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> outputMixObject,
        std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> playerObject,
        SLEqualizerItf equalizer,
        SLPlayItf playerInterface,
        const PlaybackConfiguration& config,
        const std::string& name);

    /**
     * Callback method called by the @c AndroidSLESMediaQueue when there is a queue event.
     *
     * @param event The event that has occurred.
     * @param reason A description of what triggered the error. It can be an empty string depending on the event
     * triggered.
     * @param id The id associated to the request that generated this event. The event will be ignored if the id doesn't
     * match the current request id.
     */
    void onQueueEvent(AndroidSLESMediaQueue::QueueEvent event, const std::string& reason, const SourceId& id);

    /**
     * Internal method used during object creation to register the prefetch status. We don't abort the media player
     * create if the callback cannot be registered.
     *
     * @return @c true if the registration is successful, @c false otherwise.
     */
    bool registerPrefetchStatusCallback();

    /**
     * Internal method used to create a new media queue and increment the request id.
     *
     * @param inputController A pointer to a valid decoder object that is used to decode the input into raw format.
     * @param playlistParser Optional pointer to the new playlist parser.
     * @param offset The initial playback position.This is used to compute the overall media position during playback.
     * @return The @c SourceId that represents the source being handled as a result of this call. @c ERROR will be
     * returned if the source failed to be set.
     */
    SourceId configureNewRequest(
        std::unique_ptr<FFmpegInputControllerInterface> inputController,
        std::shared_ptr<avsCommon::utils::playlistParser::IterativePlaylistParserInterface> playlistParser = nullptr,
        std::chrono::milliseconds offset = std::chrono::milliseconds(0));

    /**
     * Implements the stop media player logic. This method should only be called after acquiring @c m_mutex.
     *
     * @return @c true if the call succeeded, in which case a callback will be made, or @c false otherwise.
     */
    bool stopLocked();

    /**
     * Initializes equalizer
     *
     * @return True if initialization succeeded, false otherwise.
     */
    bool initializeEqualizer();

    /**
     * Convert the buffer size to media playback duration based on the raw audio settings.
     *
     * This method is used to estimate playback position according to the last buffer read. This is used as a
     * workaround to get the playback position since android seems to have bugs in both APIs available on OpenSL ES
     * standard.
     *
     * <a href=https://groups.google.com/d/topic/android-ndk/BMM0ete7XZg/discussion>[1]</a>
     * <a href=https://groups.google.com/d/topic/android-ndk/Ez5_nvAmhE8/discussion>[2]</a>.
     *
     * @param sizeBytes The size in bytes to be converted to playback length.
     * @return The media playback length represented by the given size.
     */
    static std::chrono::milliseconds computeDuration(size_t sizeBytes);

    /// Mutex used to synchronize @c request creation.
    std::mutex m_requestMutex;

    /// Mutex used to synchronize media player operations.
    std::mutex m_operationMutex;

    /// Used to create objects that can fetch remote HTTP content.
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;

    /// The current source id.
    SourceId m_sourceId;

    /// Save the initial media offset to compute total offset.
    std::chrono::milliseconds m_initialOffset;

    /// The speaker object that can be used to control the media player instance volume.
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> m_speaker;

    /// Keep a pointer to the OpenSL ES engine to guarantee that it doesn't get destroyed before other OpenSL ES
    /// objects.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> m_engine;

    /// Pointer to the OpenSL ES output mix object. The player relies on the output mix object.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> m_outputMixObject;

    /// The media player OpenSL ES object.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> m_playerObject;

    /// The media player observer which can be nullptr.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> m_observer;

    /// Equalizer OpenSL ES interface.
    SLEqualizerItf m_equalizer;

    /// OpenSL ES equalizer bands sorted by frequency in ascending order.
    std::vector<int> m_growingFrequenceBandMap;

    /// Equalizer band mapper to map AVS bands into OpenSL ES bands.
    std::shared_ptr<equalizer::EqualizerBandMapperInterface> m_bandMapper;

    /// Number of equalizer bands supported by the device.
    int m_numberOfEqualizerBands;

    /// Minimum band level supported by equalizer
    int m_minBandLevel;

    /// Maximum band level supported by equalizer
    int m_maxBandLevel;

    /// The media player OpenSL ES interface.
    SLPlayItf m_player;

    // The android media player configuration.
    PlaybackConfiguration m_config;

    /// The OpenSL ES interface to get the prefetch status information. This interface is optional and the media player
    /// should be functional without the prefetch information.
    SLPrefetchStatusItf m_prefetchStatus;

    /// The buffer media queue.
    std::shared_ptr<AndroidSLESMediaQueue> m_mediaQueue;

    /// The playlist parser requires explicit @c abort() call to ensure that currently ongoing parsing will stop.
    /// @note Pointer may be nullptr.
    std::shared_ptr<avsCommon::utils::playlistParser::IterativePlaylistParserInterface> m_playlistParser;

    /// Flag that indicates that the media queue is almost done playing. We ignore the underflow events when this
    /// flag is @c true.
    bool m_almostDone;

    /// Flag that indicates that the media player has been shutdown. Once shutdown is completed, playback control
    /// operations will fail.
    bool m_hasShutdown;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_H_

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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONMANAGER_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONMANAGER_H_

#include <atomic>
#include <mutex>
#include <unordered_map>

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include <Captions/CaptionFrame.h>
#include <Captions/CaptionFrameParseListenerInterface.h>
#include <Captions/CaptionLine.h>
#include <Captions/CaptionManagerInterface.h>
#include <Captions/CaptionParserInterface.h>
#include <Captions/CaptionTimingAdapter.h>
#include <Captions/TimingAdapterFactory.h>

namespace alexaClientSDK {
namespace captions {

/**
 * This class is the primary mediator between unprocessed captions content, one or more caption parsers, and an
 * implementation of the @c CaptionPresenterInterface.
 *
 * This class:
 * - routes unparsed caption data to the caption parser
 * - wraps caption text according to how much can fit on the screen, based on CaptionPresenterInterface#getWrapIndex()
 * - notifies a @c CaptionPresenterInterface when and for how long each @c CaptionFrame should be shown
 * - monitors media players to watch for when captions should be shown and hidden
 * - may receive captions from multiple sources in parallel; caption focus will match with the originating media
 *   player's state.
 * - is designed to work with en-US. While other languages may work fine, the breaks between words may be off, or
 *   the line wraps may not be accurate. If improved locale support is required, then the line break logic in the
 *   @c CaptionManager::onParsed() method should be modified to use a locale-safe method for determining the break
 *   points between words. One such method is supplied through ICU's BreakIterator:
 *   http://icu-project.org/apiref/icu4c/classicu_1_1BreakIterator.html
 */

class CaptionManager
        : public CaptionManagerInterface
        , public CaptionFrameParseListenerInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<CaptionManager> {
public:
    /**
     * Creates a @c CaptionManager as as an observer of the provided @c MediaPlayerInterfaces so that playback state of
     * the media players can be kept in sync with the captioned media.
     *
     * @param parser Parsing implementation to use for parsing captions content.
     * @param timingAdapterGenerator The generator used to produce @c CaptionTimingAdapterInterface objects on demand.
     * @return A ready to use CaptionManager.
     */
    static std::shared_ptr<CaptionManager> create(
        std::shared_ptr<CaptionParserInterface> parser,
        std::shared_ptr<TimingAdapterFactory> timingAdapterFactory = nullptr);

    /// @name RequiresShutdown methods
    /// @{
    void doShutdown() override;
    ///@}

    /// @name CaptionManagerInterface methods
    /// @{
    void onCaption(CaptionFrame::MediaPlayerSourceId sourceId, const CaptionData& captionData) override;
    void setCaptionPresenter(const std::shared_ptr<CaptionPresenterInterface>& presenter) override;
    void setMediaPlayers(
        const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayers) override;
    void addMediaPlayer(
        const std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>& mediaPlayer) override;
    void removeMediaPlayer(
        const std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>& mediaPlayer) override;
    bool isEnabled() const override;
    ///@}

    /// @name CaptionFrameParseListenerInterface methods
    /// @{
    void onParsed(const CaptionFrame& captionFrame) override;
    ///@}

    /// @name MediaPlayerObserverInterface methods
    /// @{
    void onPlaybackStarted(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackFinished(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackError(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackPaused(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackResumed(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStopped(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onFirstByteRead(
        CaptionFrame::MediaPlayerSourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override{};
    ///@}

private:
    /**
     * Constructor.
     *
     * @param parser Parsing implementation to use for parsing captions content.
     * @param timingAdapterFactory The generator used to produce @c CaptionTimingAdapterInterface objects on demand.
     */
    CaptionManager(
        std::shared_ptr<CaptionParserInterface> parser,
        std::shared_ptr<TimingAdapterFactory> timingAdapterFactory);

    /**
     * Helper function to log media state handling failure.
     * @param event The event name to log.
     * @param reason The reason for the state not being handled.
     * @param id The ID of the media source not being handled.
     */
    void logMediaStateNotHandled(
        const std::string& event,
        const std::string& reason,
        CaptionFrame::MediaPlayerSourceId id);

    /**
     * A map of @c CaptionTimingAdapter objects by the media source ID they are responsible for.
     *
     * This ensures that for each SourceID, there is a single CaptionTimingAdapter responsible for handling the timing
     * and display of the appropriate CaptionFrames.
     */
    std::unordered_map<CaptionFrame::MediaPlayerSourceId, std::shared_ptr<CaptionTimingAdapterInterface>>
        m_timingAdaptersBySourceIds;

    /// The presenter which handles the measuring and display of captions.
    std::shared_ptr<CaptionPresenterInterface> m_presenter;

    /// The parsing implementation to convert raw caption data into @c CaptionFrame objects.
    std::shared_ptr<CaptionParserInterface> m_parser;

    /// The generator instance to produce @c CaptionTimingAdapter objects on demand.
    std::shared_ptr<TimingAdapterFactory> m_timingFactory;

    /// The media players whose playback states will be used to keep playing media in sync with the associated captions.
    std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> m_mediaPlayers;

    /// Mutex for accessing the unordered_map objects, m_captionIdCounter, and m_mediaPlayers vector.
    std::mutex m_mutex;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_CAPTIONMANAGER_H_

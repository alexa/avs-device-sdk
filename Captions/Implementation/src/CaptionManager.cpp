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
#include <iterator>
#include <utility>

#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include <Captions/CaptionManager.h>
#include <Captions/CaptionTimingAdapter.h>
#include <Captions/TimingAdapterFactory.h>

namespace alexaClientSDK {
namespace captions {

using namespace alexaClientSDK::avsCommon::utils::mediaPlayer;

/// String to identify log entries originating from this file.
static const std::string TAG = "CaptionManager";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

CaptionManager::CaptionManager(
    std::shared_ptr<CaptionParserInterface> parser,
    std::shared_ptr<TimingAdapterFactory> timingAdapterFactory) :
        RequiresShutdown{TAG},
        m_parser{parser},
        m_timingFactory{timingAdapterFactory} {
}

std::shared_ptr<CaptionManager> CaptionManager::create(
    std::shared_ptr<CaptionParserInterface> parser,
    std::shared_ptr<TimingAdapterFactory> timingAdapterFactory) {
    ACSDK_DEBUG7(LX(__func__));

    if (!parser) {
        ACSDK_ERROR(LX("captionManagerCreateFailed").d("reason", "captionParserIsNull"));
        return nullptr;
    }
    if (!timingAdapterFactory) {
        timingAdapterFactory = std::make_shared<TimingAdapterFactory>();
    }

    auto captionManager = std::shared_ptr<CaptionManager>(new CaptionManager(parser, timingAdapterFactory));
    parser->addListener(captionManager);
    return captionManager;
}

void CaptionManager::doShutdown() {
    ACSDK_DEBUG7(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& player : m_mediaPlayers) {
        player->removeObserver(shared_from_this());
        player.reset();
    }

    for (auto& adapterPair : m_timingAdaptersBySourceIds) {
        auto& adapter = adapterPair.second;

        // stop the timing adapter
        adapter->reset();

        // release the shared_ptr reference
        adapter.reset();
    }
    m_timingAdaptersBySourceIds.clear();
    m_parser.reset();
}

void CaptionManager::onCaption(CaptionFrame::MediaPlayerSourceId sourceId, const CaptionData& captionData) {
    ACSDK_DEBUG7(LX(__func__));

    // Mutex is not locked before parsing because:
    // - there's no guarantee on how long the parse function will take to return
    // - order doesn't matter as far as which media source's captions gets parsed first
    ACSDK_DEBUG5(LX("sendingCaptionDataToParser").d("sourceId", sourceId));
    m_parser->parse(sourceId, captionData);
}

void CaptionManager::setCaptionPresenter(const std::shared_ptr<CaptionPresenterInterface>& presenter) {
    ACSDK_DEBUG7(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_presenter = presenter;
}

void CaptionManager::setMediaPlayers(const std::vector<std::shared_ptr<MediaPlayerInterface>>& mediaPlayers) {
    ACSDK_DEBUG7(LX(__func__));

    if (mediaPlayers.empty()) {
        ACSDK_ERROR(LX("setMediaPlayersFailed").d("reason", "noMediaPlayersAvailable"));
        return;
    }

    for (auto const& player : mediaPlayers) {
        if (!player) {
            ACSDK_ERROR(LX("setMediaPlayersFailed").d("reason", "mediaPlayersArgumentContainsNullptr"));
            return;
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_mediaPlayers.empty()) {
        // Media players are already present; release them before re-adding the new set of players.
        for (auto& player : m_mediaPlayers) {
            player->removeObserver(shared_from_this());
            player.reset();
        }
    }

    m_mediaPlayers = mediaPlayers;
    for (auto const& player : m_mediaPlayers) {
        player->addObserver(shared_from_this());
    }
    ACSDK_DEBUG5(LX("mediaPlayersAdded").d("count", mediaPlayers.size()));
}

void CaptionManager::onParsed(const CaptionFrame& captionFrame) {
    ACSDK_DEBUG3(LX(__func__));
    // This function must handle the text wrapping mechanic, using the @c CaptionPresenterInterface::getWrapIndex()
    // function to determine where the parsed caption text should be split into lines in order to fit on the client's
    // display. If the caption text needs to wrap, then the text is split in the space between words such that there is
    // no whitespace at the start or end of the lines of text. The maximum number of allowable line wraps is controlled
    // by the return value of CaptionFrame::getLineWrapLimit(), to prevent infinite loops, in case the client's
    // @c CaptionPresenter implementation is mis-configured.
    //
    // Once the CaptionFrame's lines of text have been adjusted to fit the screen, the onCaptionActivity method of the
    // @c CaptionPresenterInterface instance is called to display the captions.

    std::unique_lock<std::mutex> lock(m_mutex);
    auto presenterCopy = m_presenter;
    lock.unlock();
    if (!presenterCopy) {
        ACSDK_WARN(LX("parsedCaptionFrameIgnored").d("Reason", "presenterIsNull"));
        return;
    }

    // It's important to first merge the lines of the parsed caption frame so that they can later conform to the
    // presenter's wrap points. Some incoming captions prematurely wrap the lines with the assumption that the caption
    // text will be displayed on, for example, a television.
    CaptionLine line = CaptionLine::merge(captionFrame.getCaptionLines());

    // find the wrap points and build up the final lines of text
    std::vector<CaptionLine> wrappedCaptionLines;
    std::pair<bool, int> wrapIndexResult;
    bool shouldWrap = true;
    int wrapIndex = 0;
    int lineWrapIterationCount = 0;
    std::string lineText = line.text;
    while (shouldWrap && lineWrapIterationCount < CaptionFrame::getLineWrapLimit()) {
        wrapIndexResult = presenterCopy->getWrapIndex(line);
        shouldWrap = wrapIndexResult.first;
        if (shouldWrap) {
            wrapIndex = wrapIndexResult.second;
            // Attempt to split at the first available break between words before the requested wrap point.
            for (size_t i = wrapIndex; i > 0; i--) {
                if (' ' == lineText[i]) {
                    wrapIndex = i;
                    break;
                }
            }
            // No suitable wrap point found, so it's necessary to split in the middle of the word.
            if (0 == wrapIndex) {
                wrapIndex = wrapIndexResult.second;
            }

            // CaptionLine::splitAtTextIndex() should return at least one, but at most two elements in the returned
            // vector. If one line was returned then no splitting was done, but if two lines are present then the second
            // line should be checked with the presenter to see if it is also too long to fit on a single line.
            std::vector<CaptionLine> splitLines = line.splitAtTextIndex(wrapIndex);
            if (splitLines.size() == 1) {
                wrappedCaptionLines.emplace_back(splitLines.front());
            } else if (splitLines.size() == 2) {
                wrappedCaptionLines.emplace_back(splitLines.front());
                line = splitLines.back();
            } else {
                ACSDK_WARN(LX("unexpectedLineSplitResult").d("wrapIndex", wrapIndex).d("lineCount", splitLines.size()));
                for (const auto& splitLine : splitLines) {
                    wrappedCaptionLines.emplace_back(splitLine);
                }
            }
        }
        lineText = line.text;
        lineWrapIterationCount++;
    }

    if (lineWrapIterationCount > CaptionFrame::getLineWrapLimit()) {
        ACSDK_WARN(LX("exceededLineWrapLimit").d("LineWrapLimit", CaptionFrame::getLineWrapLimit()));
    }

    // add the remaining unwrapped line
    if (!line.text.empty()) {
        wrappedCaptionLines.emplace_back(line);
    }

    // Build up the new caption frame based on the new caption lines
    CaptionFrame displayFrame = CaptionFrame(
        captionFrame.getSourceId(), captionFrame.getDuration(), captionFrame.getDelay(), wrappedCaptionLines);

    // look up or create a new timing adapter for the media source ID.
    lock.lock();
    CaptionFrame::MediaPlayerSourceId sourceId = captionFrame.getSourceId();

    ACSDK_DEBUG5(LX("sendingCaptionToTimingAdapter").d("mediaPlayerSourceId", sourceId));
    std::shared_ptr<CaptionTimingAdapterInterface> timingAdapterCopy;
    if (m_timingAdaptersBySourceIds.count(sourceId) == 0) {
        timingAdapterCopy = m_timingFactory->getTimingAdapter(presenterCopy);
        m_timingAdaptersBySourceIds.emplace(sourceId, timingAdapterCopy);
    } else {
        timingAdapterCopy = m_timingAdaptersBySourceIds.at(sourceId);
    }
    if (!timingAdapterCopy) {
        ACSDK_ERROR(LX("presentCaptionFrameFailed").d("reason", "captionTimingAdapterIsNull"));
        return;
    }
    timingAdapterCopy->queueForDisplay(displayFrame);
    lock.unlock();
    ACSDK_DEBUG5(LX("finishedOnParsed"));
}

void CaptionManager::onPlaybackStarted(CaptionFrame::MediaPlayerSourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG3(LX(__func__).d("id", id));

    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_timingAdaptersBySourceIds.find(id);
    if (itr != m_timingAdaptersBySourceIds.end()) {
        itr->second->start();
    } else {
        logMediaStateNotHandled(__func__, "timingAdapterNotFound", id);
    }
}

void CaptionManager::onPlaybackFinished(CaptionFrame::MediaPlayerSourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG3(LX(__func__).d("id", id));

    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_timingAdaptersBySourceIds.find(id);
    if (itr != m_timingAdaptersBySourceIds.end()) {
        ACSDK_DEBUG5(LX("resettingTimingAdapter").d("sourceId", itr->first));
        itr->second.reset();
        m_timingAdaptersBySourceIds.erase(itr);
    } else {
        logMediaStateNotHandled(__func__, "timingAdapterNotFound", id);
    }
    m_parser->releaseResourcesFor(id);
}

void CaptionManager::onPlaybackError(
    CaptionFrame::MediaPlayerSourceId id,
    const ErrorType& type,
    std::string error,
    const MediaPlayerState&) {
    ACSDK_DEBUG3(LX(__func__).d("type", type).d("error", error).d("id", id));

    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_timingAdaptersBySourceIds.find(id);
    if (itr != m_timingAdaptersBySourceIds.end()) {
        itr->second->stop();
        m_timingAdaptersBySourceIds.erase(itr);
    } else {
        logMediaStateNotHandled(__func__, "timingAdapterNotFound", id);
    }
    m_parser->releaseResourcesFor(id);
}

void CaptionManager::onPlaybackPaused(CaptionFrame::MediaPlayerSourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG3(LX(__func__).d("id", id));

    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_timingAdaptersBySourceIds.find(id);
    if (itr != m_timingAdaptersBySourceIds.end()) {
        itr->second->pause();
    } else {
        logMediaStateNotHandled(__func__, "timingAdapterNotFound", id);
    }
}

void CaptionManager::onPlaybackResumed(CaptionFrame::MediaPlayerSourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG3(LX(__func__).d("id", id));

    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_timingAdaptersBySourceIds.find(id);
    if (itr != m_timingAdaptersBySourceIds.end()) {
        itr->second->start();
    } else {
        logMediaStateNotHandled(__func__, "timingAdapterNotFound", id);
    }
}

void CaptionManager::onPlaybackStopped(CaptionFrame::MediaPlayerSourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG3(LX(__func__).d("id", id));

    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_timingAdaptersBySourceIds.find(id);
    if (itr != m_timingAdaptersBySourceIds.end()) {
        itr->second->stop();
    } else {
        logMediaStateNotHandled(__func__, "timingAdapterNotFound", id);
    }
}

void CaptionManager::logMediaStateNotHandled(
    const std::string& event,
    const std::string& reason,
    CaptionFrame::MediaPlayerSourceId id) {
    ACSDK_WARN(LX("mediaStateNotHandled")
                   .d("mediaAction", event)
                   .d("Reason", reason)
                   .d("SourceId", id)
                   .d("idCount", m_timingAdaptersBySourceIds.count(id))
                   .d("timingAdaptersPresent", m_timingAdaptersBySourceIds.size()));
}

}  // namespace captions
}  // namespace alexaClientSDK

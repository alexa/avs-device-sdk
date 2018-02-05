/*
 * UrlSource.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstring>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "MediaPlayer/UrlSource.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::playlistParser;

/// String to identify log entries originating from this file.
static const std::string TAG("UrlSource");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<UrlSource> UrlSource::create(
    PipelineInterface* pipeline,
    std::shared_ptr<PlaylistParserInterface> playlistParser,
    const std::string& url) {
    if (!pipeline) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPipeline"));
        return nullptr;
    }
    if (!playlistParser) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPlaylistParser"));
        return nullptr;
    }
    ACSDK_DEBUG9(LX("UrlSourceCreate").sensitive("url", url));
    std::shared_ptr<UrlSource> result(new UrlSource(pipeline, playlistParser, url));
    if (result->init()) {
        return result;
    }
    return nullptr;
}

UrlSource::UrlSource(
    PipelineInterface* pipeline,
    std::shared_ptr<PlaylistParserInterface> playlistParser,
    const std::string& url) :
        SourceInterface("UrlSource"),
        m_url{url},
        m_playlistParser{playlistParser},
        m_hasReceivedAPlaylistCallback{false},
        m_isValid{true},
        m_pipeline{pipeline} {
}

bool UrlSource::init() {
    ACSDK_DEBUG(LX("initCalledForUrlSource"));
    /*
     * The reason we are excluding M3U8 from parsing is because GStreamer is able to handle M3U8 playlists and because
     * we've had trouble getting GStreamer to play Audible after parsing the Audible playlist into individual URLs that
     * point to audio. Once we implement our own full HTTP client, this may be removed.
     */
    if (!m_playlistParser->parsePlaylist(m_url, shared_from_this(), {PlaylistParserInterface::PlaylistType::M3U8})) {
        ACSDK_ERROR(LX("initFailed").d("reason", "startingParsePlaylistFailed"));
        return false;
    }

    auto decoder = gst_element_factory_make("uridecodebin", "decoder");
    if (!decoder) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createDecoderElementFailed"));
        return false;
    }

    if (!gst_bin_add(GST_BIN(m_pipeline->getPipeline()), decoder)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "addingDecoderToPipelineFailed"));
        gst_object_unref(decoder);
        return false;
    }

    m_pipeline->setAppSrc(nullptr);
    m_pipeline->setDecoder(decoder);

    return true;
}

bool UrlSource::hasAdditionalData() {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_url.empty()) {
        return false;
    }
    g_object_set(m_pipeline->getDecoder(), "uri", m_url.c_str(), NULL);
    return true;
}

bool UrlSource::handleEndOfStream() {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_audioUrlQueue.empty()) {
        m_url = m_audioUrlQueue.front();
        m_audioUrlQueue.pop();
    } else {
        m_url.clear();
    }
    return true;
}

void UrlSource::preprocess() {
    // Waits until at least one callback has occurred from the PlaylistParser
    m_playlistParsedPromise.get_future().get();
    /*
     * TODO: Reset the playlistParser in a better place once the thread model of MediaPlayer
     * is simplified [ACSDK-422].
     *
     * This must be called from a thread not in the UrlSource/PlaylistParser loop
     * to prevent a thread from joining itself.
     */
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_audioUrlQueue.empty()) {
        ACSDK_ERROR(LX("preprocess").d("reason", "noValidUrls"));
        return;
    }
    m_url = m_audioUrlQueue.front();
    m_audioUrlQueue.pop();

    if (!m_isValid) {
        return;
    }
    g_object_set(m_pipeline->getDecoder(), "uri", m_url.c_str(), "use-buffering", true, NULL);
}

bool UrlSource::isPlaybackRemote() const {
    return true;
}

void UrlSource::onPlaylistEntryParsed(
    int requestId,
    std::string url,
    avsCommon::utils::playlistParser::PlaylistParseResult parseResult,
    std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock{m_mutex};
    switch (parseResult) {
        ACSDK_DEBUG9(LX("onPlaylistParsed").d("parseResult", parseResult));
        case avsCommon::utils::playlistParser::PlaylistParseResult::ERROR:
            ACSDK_ERROR(LX("parseError").sensitive("url", url));
            break;
        case avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS:
        case avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING:
            ACSDK_DEBUG9(LX("urlParsedSuccessfully").sensitive("url", url));
            m_audioUrlQueue.push(url);
            break;
        default:
            ACSDK_ERROR(LX("onPlaylistParsedError").d("reason", "unknownParseResult"));
            break;
    }

    if (!m_hasReceivedAPlaylistCallback) {
        m_hasReceivedAPlaylistCallback = true;
        m_playlistParsedPromise.set_value();
    }
}

void UrlSource::doShutdown() {
    ACSDK_DEBUG9(LX("shutdownCalled"));
    std::unique_lock<std::mutex> lock{m_mutex};
    auto ptr = m_playlistParser;
    if (m_isValid) {
        m_isValid = false;
    }
    m_playlistParser.reset();
    /*
     * Make sure the m_playlistParser pointer is reset while not holding the lock to avoid potential deadlocks.
     */
    lock.unlock();
    ptr.reset();
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

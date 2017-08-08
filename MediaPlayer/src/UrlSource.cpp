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
            m_url{url},
            m_playlistParser{playlistParser},
            m_pipeline{pipeline} {
}

bool UrlSource::init() {
    ACSDK_DEBUG(LX("initCalledForUrlSource"));

    m_playlistParser->parsePlaylist(m_url, shared_from_this());

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
    return !m_url.empty();
}

bool UrlSource::handleEndOfStream() {
    m_url.clear();
    // TODO [ACSDK-419] Solidify contract that the parsed Urls will not be empty.
    while (!m_audioUrlQueue.empty()) {
        std::string url = m_audioUrlQueue.front();
        m_audioUrlQueue.pop();
        if (url.empty()) {
            ACSDK_INFO(LX("handleEndOfStream").d("info", "emptyUrlFound"));
            continue;
        }
        m_url = url;
    }

    if (!m_url.empty()) {
        g_object_set(m_pipeline->getDecoder(), "uri", m_url.c_str(), NULL);
     }
    return true;
}

void UrlSource::preprocess() {
    m_audioUrlQueue = m_playlistParsedPromise.get_future().get();
    /*
     * TODO: Reset the playlistParser in a better place once the thread model of MediaPlayer
     * is simplified [ACSDK-422].
     *
     * This must be called from a thread not in the UrlSource/PlaylistParser loop
     * to prevent a thread from joining itself.
     */
    m_playlistParser.reset();

    if (m_audioUrlQueue.empty()) {
        ACSDK_ERROR(LX("preprocess").d("reason", "noValidUrls"));
        return;
    }
    m_url = m_audioUrlQueue.front();
    m_audioUrlQueue.pop();

    g_object_set(m_pipeline->getDecoder(),
            "uri", m_url.c_str(),
            "use-buffering", true,
            NULL);
}

void UrlSource::onPlaylistParsed(std::string initialUrl, std::queue<std::string> urls, PlaylistParseResult parseResult) {
    ACSDK_DEBUG(LX("onPlaylistParsed").d("parseResult", parseResult).d("numUrlsParsed", urls.size()));
    // The parse was unrecognized by the parser, could be a single song. Attempt to play.
    if (urls.size() == 0) {
        urls.push(initialUrl);
    }
    m_playlistParsedPromise.set_value(urls);
};

} // namespace mediaPlayer
} // namespace alexaClientSDK

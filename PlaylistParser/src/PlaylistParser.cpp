/*
 * PlaylistParser.cpp
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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "PlaylistParser/PlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("PlaylistParser");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<PlaylistParser> PlaylistParser::create() {
    ACSDK_DEBUG9(LX("createCalled"));
    std::unique_ptr<PlaylistParser> playlistParser(new PlaylistParser());
    return playlistParser;
}

PlaylistParser::~PlaylistParser() {
    ACSDK_DEBUG9(LX("destructorCalled"));
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_isShuttingDown = true;
        for (auto info : m_playlistInfoQueue) {
            g_signal_handler_disconnect(info->parser, info->playlistStartedHandlerId);
            g_signal_handler_disconnect(info->parser, info->entryParsedHandlerId);
            info->observer->onPlaylistParsed(
                    info->playlistUrl,
                    info->urlQueue,
                    avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_CANCELLED);
        }
        m_playlistInfoQueue.clear();
        m_playlistInfo = nullptr;
    }
    m_wakeParsingLoop.notify_one();
    if (m_parserThread.joinable()) {
        m_parserThread.join();
    }
}

bool PlaylistParser::parsePlaylist(const std::string& url,
            std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer) {
    ACSDK_DEBUG9(LX("parsePlaylist").d("url", url));

    if (url.empty()) {
        ACSDK_ERROR(LX("parsePlaylistFailed").d("reason","emptyUrl"));
        return false;
    }

    if (!observer) {
        ACSDK_ERROR(LX("parsePlaylistFailed").d("reason","observerIsNullptr"));
        return false;
    }

    auto playlistInfo = createPlaylistInfo(url, observer);
    if (!playlistInfo) {
        ACSDK_ERROR(LX("parsePlaylistFailed").d("reason", "cannotCreateNewPlaylistInfo"));
        return false;
    }

    playlistInfo->playlistUrl = url;
    playlistInfo->observer = observer;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playlistInfoQueue.push_back(playlistInfo);
        m_wakeParsingLoop.notify_one();
    }
    return true;
}

PlaylistParser::PlaylistInfo::PlaylistInfo(TotemPlParser* plParser)
        :
        parser{plParser},
        playlistStartedHandlerId{0},
        entryParsedHandlerId{0} {
}


PlaylistParser::PlaylistInfo::~PlaylistInfo() {
    g_clear_object(&parser);
}

PlaylistParser::PlaylistParser()
        :
        m_isParsingActive{false},
        m_isShuttingDown{false} {
    m_parserThread = std::thread(&PlaylistParser::parsingLoop, this);
}

std::shared_ptr<PlaylistParser::PlaylistInfo> PlaylistParser::createPlaylistInfo(
        const std::string& url,
        std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer) {
    ACSDK_DEBUG9(LX("createPlaylistInfo"));

    TotemPlParser* parser = totem_pl_parser_new();
    if (!parser) {
        ACSDK_ERROR(LX("createPlaylistInfoFailed").d("reason", "cannotCreateNewParser"));
        return nullptr;
    }

    std::shared_ptr<PlaylistParser::PlaylistInfo> playlistInfo = std::make_shared<PlaylistInfo>(parser);

    g_object_set(parser, "recurse", TRUE, "disable-unsafe", TRUE, "force", TRUE, NULL);

    playlistInfo->playlistStartedHandlerId = g_signal_connect(G_OBJECT(parser), "playlist-started",
            G_CALLBACK(onPlaylistStarted), this);
    if (!playlistInfo->playlistStartedHandlerId) {
        ACSDK_ERROR(LX("createPlaylistInfoFailed").d("reason", "cannotConnectPlaylistStartedSignal"));
        return nullptr;
    }

    playlistInfo->entryParsedHandlerId = g_signal_connect(G_OBJECT(parser), "entry-parsed",
            G_CALLBACK(onEntryParsed), this);
    if (!playlistInfo->entryParsedHandlerId) {
        ACSDK_ERROR(LX("createPlaylistInfoFailed").d("reason", "cannotConnectEntryParsedSignal"));
        g_signal_handler_disconnect(playlistInfo->parser, playlistInfo->playlistStartedHandlerId);
        return nullptr;
    }

    return playlistInfo;
}

void PlaylistParser::onPlaylistStarted (TotemPlParser *parser, gchar *url, TotemPlParserMetadata *metadata,
            gpointer pointer) {
    ACSDK_DEBUG9(LX("onPlaylistStarted").d("url", url));
}

void PlaylistParser::onEntryParsed(TotemPlParser *parser, gchar *url, TotemPlParserMetadata *metadata,
            gpointer pointer) {
    ACSDK_DEBUG9(LX("onEntryParsed").d("url", url));
    auto playlistParser = static_cast<PlaylistParser *>(pointer);
    if (playlistParser && playlistParser->m_playlistInfo){
        playlistParser->handleOnEntryParsed(url, playlistParser->m_playlistInfo);
    }
}

void PlaylistParser::onParseComplete(GObject* parser, GAsyncResult* result, gpointer pointer) {
    ACSDK_DEBUG9(LX("onParseComplete"));
    auto playlistParser = static_cast<PlaylistParser *>(pointer);
    if (playlistParser && playlistParser->m_playlistInfo) {
        playlistParser->handleOnParseComplete(result, playlistParser->m_playlistInfo);
    }
}

void PlaylistParser::handleParsingLocked(std::unique_lock<std::mutex>& lock) {
    ACSDK_DEBUG9(LX("handleParsingLocked"));
    m_playlistInfo = m_playlistInfoQueue.front();
    m_isParsingActive = true;
    lock.unlock();
    totem_pl_parser_parse_async(m_playlistInfo->parser, m_playlistInfo->playlistUrl.c_str(), FALSE, NULL,
            onParseComplete, this);
}

void PlaylistParser::handleOnEntryParsed(gchar *url, std::shared_ptr<PlaylistInfo> playlistInfo) {
    ACSDK_DEBUG9(LX("handleOnEntryParsed").d("url", url));
    std::lock_guard<std::mutex> lock(playlistInfo->mutex);
    playlistInfo->urlQueue.push(url);
}

void PlaylistParser::handleOnParseComplete(GAsyncResult* result, std::shared_ptr<PlaylistInfo> playlistInfo) {
    ACSDK_DEBUG9(LX("handleOnParseComplete"));
    GError *error = nullptr;
    auto parserResult = totem_pl_parser_parse_finish (playlistInfo->parser, result, &error);
    std::queue<std::string> urlQueue;

    {
        std::lock_guard<std::mutex> lock(playlistInfo->mutex);
        urlQueue = playlistInfo->urlQueue;
    }

    playlistInfo->observer->onPlaylistParsed(
            playlistInfo->playlistUrl,
            urlQueue,
            mapResult(parserResult));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_playlistInfoQueue.empty()) {
            m_playlistInfoQueue.pop_front();
        }
        m_isParsingActive = false;
        m_wakeParsingLoop.notify_one();
    }
}

avsCommon::utils::playlistParser::PlaylistParseResult PlaylistParser::mapResult(TotemPlParserResult result) {
    switch(result) {
        case TOTEM_PL_PARSER_RESULT_SUCCESS:
            ACSDK_DEBUG9(LX("playlistParsingSuccessful"));
            return avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_SUCCESS;
        case TOTEM_PL_PARSER_RESULT_UNHANDLED:
            ACSDK_DEBUG9(LX("playlistCouldNotBeHandled"));
            return avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_UNHANDLED;
        case TOTEM_PL_PARSER_RESULT_ERROR:
            ACSDK_DEBUG9(LX("playlistParsingError"));
            return avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_ERROR;
        case TOTEM_PL_PARSER_RESULT_IGNORED:
            ACSDK_DEBUG9(LX("playlistWasIgnoredDueToSchemeOrMimeType"));
            return avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_IGNORED;
        case TOTEM_PL_PARSER_RESULT_CANCELLED:
            ACSDK_DEBUG9(LX("playlistParsingWasCancelledPartWayThrough"));
            return avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_CANCELLED;
    }
    return avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_ERROR;
}

void PlaylistParser::parsingLoop() {
    auto wake = [this]() {
        return (m_isShuttingDown || (!m_playlistInfoQueue.empty() && !m_isParsingActive));
    };

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeParsingLoop.wait(lock, wake);
        if (m_isShuttingDown) {
            break;
        }
        handleParsingLocked(lock);
    }
}

} // namespace playlistParser
} // namespace alexaClientSDK

/*
 * DummyPlaylistParser.cpp
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

#include "PlaylistParser/DummyPlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("DummyPlaylistParser");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<DummyPlaylistParser> DummyPlaylistParser::create() {
    ACSDK_DEBUG9(LX("createCalled"));
    auto playlistParser = std::make_shared<DummyPlaylistParser>();
    return playlistParser;
}

bool DummyPlaylistParser::parsePlaylist(const std::string& url,
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

    // An empty queue to be passed to the observer
    std::queue<std::string> emptyUrlQueue;

    observer->onPlaylistParsed(
            url,
            emptyUrlQueue,
            avsCommon::utils::playlistParser::PlaylistParseResult::PARSE_RESULT_UNHANDLED);

    return true;
}

} // namespace playlistParser
} // namespace alexaClientSDK

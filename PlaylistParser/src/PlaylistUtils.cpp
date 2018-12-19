/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "PlaylistParser/PlaylistUtils.h"

#include <algorithm>
#include <sstream>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("PlaylistUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
static const size_t CHUNK_SIZE(1024);

/// The first line of a PLS playlist.
static const std::string PLS_PLAYLIST_HEADER = "[playlist]";

/// The beginning of a line in a PLS file indicating a URL.
static const std::string PLS_FILE = "File";

/// url scheme pattern.
static const std::string URL_END_SCHEME_PATTERN = "://";

bool extractPlaylistContent(std::unique_ptr<avsCommon::utils::HTTPContent> httpContent, std::string* content) {
    if (!content) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullString"));
        return false;
    }
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    if (!httpContent->isStatusCodeSuccess()) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed")
                        .d("reason", "badHTTPContentReceived")
                        .d("statusCode", httpContent->getStatusCode()));
        return false;
    }

    auto reader = httpContent->getDataStream()->createReader(avsCommon::utils::sds::ReaderPolicy::BLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "failedToCreateStreamReader"));
        return false;
    }
    avsCommon::avs::attachment::AttachmentReader::ReadStatus readStatus =
        avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK;
    std::string playlistContent;
    std::vector<char> buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    while (!streamClosed) {
        auto bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        switch (readStatus) {
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (bytesRead == 0) {
                    break;
                }
            /* FALL THROUGH - to add any data received even if closed */
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_TIMEDOUT:
                playlistContent.append(buffer.data(), bytesRead);
                break;
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
                // Current AttachmentReader policy renders this outcome impossible.
                ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("failure", "overrunReset"));
                break;
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("getContentFromPlaylistUrlIntoStringFailed").d("reason", "readError"));
                return false;
        }
    }
    *content = playlistContent;
    return true;
}

std::vector<std::string> parsePLSContent(const std::string& playlistURL, const std::string& content) {
    /*
     * A PLS playlist is formatted such that all URLs to play are prepended with "File'N'=", where 'N' refers to the
     * numbered URL. For example "File1=url.com ... File2="anotherurl.com".
     */
    std::vector<std::string> urlsParsed;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        removeCarriageReturnFromLine(&line);
        if (line.compare(0, PLS_FILE.length(), PLS_FILE) == 0) {
            std::string url = line.substr(line.find_first_of('=') + 1);
            if (isURLAbsolute(url)) {
                urlsParsed.push_back(url);
            } else {
                std::string absoluteURL;
                if (getAbsoluteURLFromRelativePathToURL(playlistURL, url, &absoluteURL)) {
                    urlsParsed.push_back(absoluteURL);
                }
            }
        }
    }
    return urlsParsed;
}

void removeCarriageReturnFromLine(std::string* line) {
    if (!line) {
        return;
    }
    if (!line->empty() && (line->back() == '\r' || line->back() == '\n')) {
        line->pop_back();
    }
}

bool isURLAbsolute(const std::string& url) {
    return url.find(URL_END_SCHEME_PATTERN) != std::string::npos;
}

bool getAbsoluteURLFromRelativePathToURL(std::string baseURL, std::string relativePath, std::string* absoluteURL) {
    if (!absoluteURL) {
        ACSDK_ERROR(LX("getAbsoluteURLFromRelativePathToURLFailed").d("reason", "nullAbsoluteURL"));
        return false;
    }

    auto schemeEndPosition = baseURL.find(URL_END_SCHEME_PATTERN);
    if (schemeEndPosition == std::string::npos) {
        ACSDK_ERROR(LX("getAbsoluteURLFromRelativePathToURLFailed").d("reason", "invalidBaseURL"));
        return false;
    }

    if (relativePath.empty()) {
        *absoluteURL = baseURL;
        return true;
    }

    auto searchBegin = schemeEndPosition + URL_END_SCHEME_PATTERN.size();
    if (relativePath[0] == '/') {
        // Look for first '/' after scheme://
        auto firstSlashPosition = baseURL.find_first_of("/", searchBegin);
        if (firstSlashPosition == std::string::npos) {
            ACSDK_ERROR(LX("getAbsoluteURLFromRelativePathToURLFailed").d("reason", "firstSlashNotFound"));
            return false;
        }
        baseURL.resize(firstSlashPosition);
    } else {
        auto lastSlashPosition = baseURL.find_last_of("/");
        if (lastSlashPosition == std::string::npos || lastSlashPosition < searchBegin) {
            ACSDK_ERROR(LX("getAbsoluteURLFromRelativePathToURLFailed").d("reason", "lastSlashNotFound"));
            return false;
        }
        baseURL.resize(lastSlashPosition + 1);
    }
    *absoluteURL = baseURL + relativePath;
    return true;
}

}  // namespace playlistParser
}  // namespace alexaClientSDK

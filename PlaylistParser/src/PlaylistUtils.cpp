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

#include "PlaylistParser/PlaylistUtils.h"

#include <algorithm>
#include <sstream>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::sds;

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

/// The beginning of a line in a PLS file indicating a URL.
static const std::string PLS_FILE = "File";

/// url scheme pattern.
static const std::string URL_END_SCHEME_PATTERN = "://";

/// A wait period for a polling loop that constantly check if a content fetcher finished fetching the payload or failed.
static const std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT{100};

/// Process attachment ID
static const std::string PROCESS_ATTACHMENT_ID = "download:";

/// Timeout to wait for a playlist to arrive from the content fetcher
static const std::chrono::minutes PLAYLIST_FETCH_TIMEOUT{5};

bool readFromContentFetcher(
    std::unique_ptr<HTTPContentFetcherInterface> contentFetcher,
    std::string* content,
    std::atomic<bool>* shouldShutDown) {
    ACSDK_DEBUG9(LX(__func__));
    if (!contentFetcher) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "nullContentFetcher"));
        return false;
    }

    if (!content) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "nullContent"));
        return false;
    }

    if (*shouldShutDown) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "shouldShutdown"));
        return false;
    }

    auto header = contentFetcher->getHeader(shouldShutDown);
    if (!header.successful) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "getHeaderFailed"));
        return false;
    }

    if (!isStatusCodeSuccess(header.responseCode)) {
        ACSDK_WARN(LX("readFromContentFetcherFailed")
                       .d("reason", "failedToReceiveHeader")
                       .d("statusCode", header.responseCode));
        return false;
    }

    auto stream = std::make_shared<InProcessAttachment>(PROCESS_ATTACHMENT_ID);
    std::shared_ptr<AttachmentWriter> streamWriter = stream->createWriter(WriterPolicy::BLOCKING);

    if (!contentFetcher->getBody(streamWriter)) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "getBodyFailed"));
        return false;
    }

    auto startTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
    HTTPContentFetcherInterface::State contentFetcherState = contentFetcher->getState();
    while ((PLAYLIST_FETCH_TIMEOUT > elapsedTime) &&
           (HTTPContentFetcherInterface::State::BODY_DONE != contentFetcherState) &&
           (HTTPContentFetcherInterface::State::ERROR != contentFetcherState)) {
        std::this_thread::sleep_for(WAIT_FOR_ACTIVITY_TIMEOUT);
        if (*shouldShutDown) {
            return false;
        }
        elapsedTime = std::chrono::steady_clock::now() - startTime;
        contentFetcherState = contentFetcher->getState();
    }
    if (PLAYLIST_FETCH_TIMEOUT <= elapsedTime) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "waitTimeout"));
        return false;
    }

    if (HTTPContentFetcherInterface::State::ERROR == contentFetcherState) {
        ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "receivingBodyFailed"));
        return false;
    }

    ACSDK_DEBUG9(LX("bodyReceived"));

    std::unique_ptr<AttachmentReader> reader = stream->createReader(ReaderPolicy::NONBLOCKING);

    auto readStatus = AttachmentReader::ReadStatus::OK;
    std::vector<char> buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    AttachmentReader::ReadStatus previousStatus = AttachmentReader::ReadStatus::OK_TIMEDOUT;
    ssize_t bytesReadSoFar = 0;
    size_t bytesRead = -1;
    while (!streamClosed && bytesRead != 0) {
        bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        bytesReadSoFar += bytesRead;
        if (previousStatus != readStatus) {
            ACSDK_DEBUG9(LX(__func__).d("readStatus", readStatus));
            previousStatus = readStatus;
        }
        switch (readStatus) {
            case AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (bytesRead == 0) {
                    break;
                }
                /* FALL THROUGH - to add any data received even if closed */
            case AttachmentReader::ReadStatus::OK:
            case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case AttachmentReader::ReadStatus::OK_TIMEDOUT:
                content->append(buffer.data(), bytesRead);
                break;
            case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
                // Current AttachmentReader policy renders this outcome impossible.
                ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "overrunReset"));
                break;
            case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("readFromContentFetcherFailed").d("reason", "readError"));
                return false;
        }
        if (0 == bytesRead) {
            ACSDK_DEBUG9(LX(__func__).m("alreadyReadAllBytes"));
        }
    }

    ACSDK_DEBUG9(LX("readFromContentFetcherDone").d("URL", contentFetcher->getUrl()).d("content", *content));
    return !(*content).empty();
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

/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "PlaylistParser/UrlContentToAttachmentConverter.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("UrlContentToAttachmentConverter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const std::chrono::milliseconds UNVALID_DURATION =
    avsCommon::utils::playlistParser::PlaylistParserObserverInterface::INVALID_DURATION;

std::shared_ptr<UrlContentToAttachmentConverter> UrlContentToAttachmentConverter::create(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    const std::string& url,
    std::shared_ptr<ErrorObserverInterface> observer,
    std::chrono::milliseconds startTime) {
    if (!contentFetcherFactory) {
        return nullptr;
    }
    auto thisSharedPointer = std::shared_ptr<UrlContentToAttachmentConverter>(
        new UrlContentToAttachmentConverter(contentFetcherFactory, url, observer, startTime));
    auto retVal = thisSharedPointer->m_playlistParser->parsePlaylist(url, thisSharedPointer);
    if (0 == retVal) {
        thisSharedPointer->shutdown();
        return nullptr;
    }
    return thisSharedPointer;
}

std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> UrlContentToAttachmentConverter::getAttachment() {
    return m_stream;
}

UrlContentToAttachmentConverter::UrlContentToAttachmentConverter(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
    const std::string& url,
    std::shared_ptr<ErrorObserverInterface> observer,
    std::chrono::milliseconds startTime) :
        RequiresShutdown{"UrlContentToAttachmentConverter"},
        m_desiredStreamPoint{startTime},
        m_contentFetcherFactory{contentFetcherFactory},
        m_observer{observer},
        m_shuttingDown{false},
        m_runningTotal{0},
        m_startedStreaming{false},
        m_streamWriterClosed{false} {
    m_playlistParser = PlaylistParser::create(m_contentFetcherFactory);
    m_startStreamingPointFuture = m_startStreamingPointPromise.get_future();
    m_stream = std::make_shared<avsCommon::avs::attachment::InProcessAttachment>(url);
    m_streamWriter = m_stream->createWriter(avsCommon::utils::sds::WriterPolicy::BLOCKING);
}

std::chrono::milliseconds UrlContentToAttachmentConverter::getStartStreamingPoint() {
    return m_startStreamingPointFuture.get();
}

std::chrono::milliseconds UrlContentToAttachmentConverter::getDesiredStreamingPoint() {
    return m_desiredStreamPoint;
}

void UrlContentToAttachmentConverter::onPlaylistEntryParsed(
    int requestId,
    std::string url,
    avsCommon::utils::playlistParser::PlaylistParseResult parseResult,
    std::chrono::milliseconds duration) {
    if (!m_startedStreaming) {
        if (m_desiredStreamPoint.count() > 0) {
            if (duration == UNVALID_DURATION) {
                m_startStreamingPointPromise.set_value(m_runningTotal);
                // Allow to start streaming below
            } else if (m_runningTotal + duration <= m_desiredStreamPoint) {
                m_runningTotal += duration;
                return;
            } else {
                m_startStreamingPointPromise.set_value(m_runningTotal);
                m_runningTotal += duration;
                // Allow to begin streaming below
            }
        } else {
            m_startStreamingPointPromise.set_value(std::chrono::seconds::zero());
        }
    }
    m_startedStreaming = true;
    ACSDK_DEBUG3(LX("onPlaylistEntryParsed").d("status", parseResult));
    switch (parseResult) {
        case avsCommon::utils::playlistParser::PlaylistParseResult::ERROR:
            m_executor.submit([this]() {
                ACSDK_DEBUG9(LX("closingWriter"));
                m_streamWriter->close();
                m_streamWriterClosed = true;
                std::unique_lock<std::mutex> lock{m_mutex};
                auto observer = m_observer;
                lock.unlock();
                if (observer) {
                    observer->onError();
                }
            });
            break;
        case avsCommon::utils::playlistParser::PlaylistParseResult::SUCCESS:
            m_executor.submit([this, url]() {
                if (!m_streamWriterClosed && !writeUrlContentIntoStream(url)) {
                    ACSDK_ERROR(LX("writeUrlContentToStreamFailed"));
                    std::unique_lock<std::mutex> lock{m_mutex};
                    auto observer = m_observer;
                    lock.unlock();
                    if (observer) {
                        observer->onError();
                    }
                }
                ACSDK_DEBUG9(LX("closingWriter"));
                m_streamWriter->close();
                m_streamWriterClosed = true;
            });
            break;
        case avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING:
            m_executor.submit([this, url]() {
                if (!m_streamWriterClosed && !writeUrlContentIntoStream(url)) {
                    ACSDK_ERROR(LX("writeUrlContentToStreamFailed").d("info", "closingWriter"));
                    m_streamWriter->close();
                    m_streamWriterClosed = true;
                    std::unique_lock<std::mutex> lock{m_mutex};
                    auto observer = m_observer;
                    lock.unlock();
                    if (observer) {
                        observer->onError();
                    }
                }
            });
            break;
        default:
            return;
    }
}

bool UrlContentToAttachmentConverter::writeUrlContentIntoStream(std::string url) {
    ACSDK_DEBUG9(LX("writeUrlContentIntoStream").d("info", "beginning"));

    auto contentFetcher = m_contentFetcherFactory->create(url);
    auto httpContent = contentFetcher->getContent(
        avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY, m_streamWriter);
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    if (!(*httpContent)) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "badHTTPContentReceived"));
        return false;
    }
    if (m_shuttingDown) {
        return false;
    }
    ACSDK_DEBUG9(LX("writeUrlContentIntoStreamSuccess"));
    return true;
}

void UrlContentToAttachmentConverter::doShutdown() {
    m_streamWriter->close();

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_observer.reset();
    }
    m_shuttingDown = true;
    m_executor.shutdown();
    m_playlistParser->shutdown();
    m_playlistParser.reset();
    m_streamWriter.reset();
    if (!m_startedStreaming) {
        m_startStreamingPointPromise.set_value(std::chrono::seconds::zero());
    }
}

}  // namespace playlistParser
}  // namespace alexaClientSDK

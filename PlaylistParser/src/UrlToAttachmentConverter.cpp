/*
 * UrlToAttachmentConverter.cpp
 *
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

#include "PlaylistParser/UrlToAttachmentConverter.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace playlistParser {

/// String to identify log entries originating from this file.
static const std::string TAG("UrlContentToAttachmentConverter");

/**
 * The timeout for a blocking write call to an @c AttachmentWriter. This value may be increased to decrease wakeups but
 * may also increase latency.
 */
static const std::chrono::milliseconds TIMEOUT_FOR_BLOCKING_WRITE = std::chrono::milliseconds(100);

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of bytes read from the attachment with each read in the read loop.
// Just smaller than the default megabyte size of an Attachment to allow for maximum possible write size at at time
static const size_t CHUNK_SIZE{avsCommon::avs::attachment::InProcessAttachment::SDS_BUFFER_DEFAULT_SIZE_IN_BYTES -
                               avsCommon::avs::attachment::InProcessAttachment::SDS_BUFFER_DEFAULT_SIZE_IN_BYTES / 4};

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
    switch (parseResult) {
        case avsCommon::utils::playlistParser::PlaylistParseResult::ERROR:
            ACSDK_DEBUG9(LX("onPlaylistEntryParsed").d("status", parseResult));
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
            ACSDK_DEBUG9(LX("onPlaylistEntryParsed").d("status", parseResult));
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
            ACSDK_DEBUG9(LX("onPlaylistEntryParsed").d("status", parseResult));
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
    /*
     * TODO: ACSDK-826 We currently copy from one SDS with the individual URL data into a master SDS. We could probably
     * optimize this to avoid the extra copy.
     */
    ACSDK_DEBUG9(LX("writeUrlContentIntoStream").d("info", "beginning"));
    auto contentFetcher = m_contentFetcherFactory->create(url);
    auto httpContent =
        contentFetcher->getContent(avsCommon::sdkInterfaces::HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);
    if (!httpContent) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "nullHTTPContentReceived"));
        return false;
    }
    if (!(*httpContent)) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "badHTTPContentReceived"));
        return false;
    }
    if (!httpContent->dataStream) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "badDataStream"));
        return false;
    }
    auto reader = httpContent->dataStream->createReader(avsCommon::avs::attachment::AttachmentReader::Policy::BLOCKING);
    if (!reader) {
        ACSDK_ERROR(LX("getContentFailed").d("reason", "failedToCreateStreamReader"));
        return false;
    }
    avsCommon::avs::attachment::AttachmentReader::ReadStatus readStatus =
        avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK;
    std::vector<char> buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    while (!streamClosed && !m_shuttingDown) {
        auto bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        switch (readStatus) {
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (bytesRead == 0) {
                    ACSDK_INFO(LX("readFinished").d("reason", "CLOSED"));
                    break;
                }
            /* FALL THROUGH - to add any data received even if closed */
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::OK_TIMEDOUT:
                if (!writeDataIntoStream(buffer, bytesRead)) {
                    ACSDK_ERROR(LX("writeDataIntoStreamFailed").d("reason", "writeError"));
                    return false;
                }
                break;
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case avsCommon::avs::attachment::AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("readFailed").d("status", "readError"));
                return false;
        }
    }
    if (m_shuttingDown) {
        return false;
    }
    ACSDK_DEBUG9(LX("writeUrlContentIntoStreamSuccess"));
    return true;
}

bool UrlContentToAttachmentConverter::writeDataIntoStream(const std::vector<char>& buffer, size_t numBytes) {
    avsCommon::avs::attachment::AttachmentWriter::WriteStatus writeStatus =
        avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK;

    size_t totalBytesWritten = 0;
    auto bufferStart = buffer.data();
    while (totalBytesWritten < numBytes && !m_shuttingDown) {
        // because we use a BLOCKING writer, we have to keep track of how many bytes are written per write()
        // and update the buffer pointer accordingly
        size_t bytesWritten =
            m_streamWriter->write(bufferStart, numBytes - totalBytesWritten, &writeStatus, TIMEOUT_FOR_BLOCKING_WRITE);
        bufferStart += bytesWritten;
        totalBytesWritten += bytesWritten;
        switch (writeStatus) {
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::CLOSED:
                // TODO: ACSDK-827 Replace with just the writeStatus once the << operator is added
                ACSDK_ERROR(LX("writeContentFailed").d("reason", "writeStatusCLOSED"));
                return false;
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
                ACSDK_ERROR(LX("writeContentFailed").d("reason", "writeStatusBYTESLESSTHANWORDSIZE"));
                return false;
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("writeContentFailed").d("reason", "writeStatusERRORINTERNAL"));
                return false;
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::TIMEDOUT:
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK:
                // might still have bytes to write
                continue;
            case avsCommon::avs::attachment::AttachmentWriter::WriteStatus::OK_BUFFER_FULL:
                ACSDK_ERROR(LX("writeContentFailed").d("unexpected return code", "writeStatusOK_BUFFER_FULL"));
                return false;
            default:
                ACSDK_ERROR(LX("writeContentFailed").d("reason", "unknownWriteStatus"));
                return false;
        }
    }
    if (m_shuttingDown) {
        return false;
    }
    return true;
}

void UrlContentToAttachmentConverter::doShutdown() {
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

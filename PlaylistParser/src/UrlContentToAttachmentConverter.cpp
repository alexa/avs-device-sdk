/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::playlistParser;
using namespace avsCommon::utils::sds;

/// String to identify log entries originating from this file.
static const std::string TAG("UrlContentToAttachmentConverter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// An invalid duration.
static const std::chrono::milliseconds INVALID_DURATION = std::chrono::milliseconds(-1);

/// The number of bytes read from the attachment with each read in the read loop.
static const size_t CHUNK_SIZE(1024);

/// Timeout for polling loops that check activities running on separate threads.
static const std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT{100};

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

std::shared_ptr<InProcessAttachment> UrlContentToAttachmentConverter::getAttachment() {
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
    m_stream = std::make_shared<InProcessAttachment>(url);
    m_streamWriter = m_stream->createWriter(avsCommon::utils::sds::WriterPolicy::BLOCKING);
    m_contentDecrypter = std::make_shared<ContentDecrypter>();
}

std::chrono::milliseconds UrlContentToAttachmentConverter::getStartStreamingPoint() {
    return m_startStreamingPointFuture.get();
}

std::chrono::milliseconds UrlContentToAttachmentConverter::getDesiredStreamingPoint() {
    return m_desiredStreamPoint;
}

void UrlContentToAttachmentConverter::onPlaylistEntryParsed(int requestId, PlaylistEntry playlistEntry) {
    auto parseResult = playlistEntry.parseResult;
    auto url = playlistEntry.url;
    auto duration = playlistEntry.duration;
    auto encryptionInfo = playlistEntry.encryptionInfo;
    std::vector<std::string> headers;

    if (playlistEntry.hasValidByteRange()) {
        long start, end;
        std::tie(start, end) = playlistEntry.byteRange;
        auto header = "Range: bytes=" + std::to_string(start) + '-' + std::to_string(end);
        headers.push_back(header);
    }

    // Download and cache Media Initialization Section for SAMPLE-AES content.
    if (playlistEntry.type == PlaylistEntry::Type::MEDIA_INIT_INFO &&
        encryptionInfo.method == EncryptionInfo::Method::SAMPLE_AES) {
        m_executor.submit([this, url, headers, playlistEntry]() {
            ByteVector mediaInitSection;
            if (!download(url, headers, &mediaInitSection, playlistEntry.contentFetcher)) {
                closeStreamWriter();
                notifyError();
                return;
            }
            if (!m_shuttingDown) {
                m_contentDecrypter->writeMediaInitSection(mediaInitSection, m_streamWriter);
            }
        });
        return;
    } else if (PlaylistEntry::Type::AUDIO_CONTENT == playlistEntry.type) {
        ACSDK_DEBUG3(LX("foundAudioContent"));
    }

    if (!m_startedStreaming) {
        if (m_desiredStreamPoint.count() > 0) {
            if (INVALID_DURATION == duration) {
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
    auto contentFetcher = playlistEntry.contentFetcher;
    switch (parseResult) {
        case avsCommon::utils::playlistParser::PlaylistParseResult::ERROR:
            m_executor.submit([this]() {
                ACSDK_DEBUG9(LX("closingWriter"));
                closeStreamWriter();
                notifyError();
            });
            break;
        case avsCommon::utils::playlistParser::PlaylistParseResult::FINISHED:
            m_executor.submit([this, url, headers, encryptionInfo, contentFetcher]() {
                ACSDK_DEBUG9(LX("calling writeDecryptedUrlContentIntoStream"));
                if (!m_streamWriterClosed &&
                    !writeDecryptedUrlContentIntoStream(url, headers, encryptionInfo, contentFetcher)) {
                    ACSDK_ERROR(LX("writeUrlContentToStreamFailed"));
                    notifyError();
                }
                ACSDK_DEBUG9(LX("closingWriter"));
                closeStreamWriter();
            });
            break;
        case avsCommon::utils::playlistParser::PlaylistParseResult::STILL_ONGOING:
            m_executor.submit([this, url, headers, encryptionInfo, contentFetcher]() {
                if (!m_streamWriterClosed &&
                    !writeDecryptedUrlContentIntoStream(url, headers, encryptionInfo, contentFetcher)) {
                    ACSDK_ERROR(LX("writeUrlContentToStreamFailed").d("info", "closingWriter"));
                    closeStreamWriter();
                    notifyError();
                }
            });
            break;
        default:
            return;
    }
}

void UrlContentToAttachmentConverter::closeStreamWriter() {
    ACSDK_DEBUG(LX(__func__));
    m_streamWriter->close();
    m_streamWriterClosed = true;
}

void UrlContentToAttachmentConverter::notifyError() {
    std::unique_lock<std::mutex> lock{m_mutex};
    auto observer = m_observer;
    lock.unlock();
    if (observer) {
        observer->onError();
    }
}

bool UrlContentToAttachmentConverter::writeDecryptedUrlContentIntoStream(
    std::string url,
    std::vector<std::string> headers,
    EncryptionInfo encryptionInfo,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher) {
    ACSDK_DEBUG9(LX("writeDecryptedUrlContentIntoStream").d("info", "beginning"));

    auto hasValidEncryption = shouldDecrypt(encryptionInfo);
    if (hasValidEncryption) {
        ByteVector content;
        if (!download(url, headers, &content, contentFetcher)) {
            ACSDK_ERROR(LX("writeDecryptedUrlContentIntoStreamFailed").d("reason", "downloadContentFailed"));
            return false;
        }

        ByteVector key;
        if (!download(encryptionInfo.keyURL, std::vector<std::string>(), &key, contentFetcher)) {
            ACSDK_ERROR(LX("writeDecryptedUrlContentIntoStreamFailed").d("reason", "downloadEncryptionKeyFailed"));
            return false;
        }

        if (!m_shuttingDown && !m_contentDecrypter->decryptAndWrite(content, key, encryptionInfo, m_streamWriter)) {
            ACSDK_ERROR(LX("writeDecryptedUrlContentIntoStreamFailed").d("reason", "decryptAndWriteFailed"));
            return false;
        }
    } else {
        if (!download(url, headers, m_streamWriter, contentFetcher)) {
            ACSDK_ERROR(LX("writeDecryptedUrlContentIntoStreamFailed").d("reason", "downloadFailed"));
            return false;
        }
    }

    ACSDK_DEBUG9(LX("writeDecryptedUrlContentIntoStreamSuccess"));
    return true;
}

bool UrlContentToAttachmentConverter::shouldDecrypt(const EncryptionInfo& encryptionInfo) const {
    return encryptionInfo.isValid() && encryptionInfo.method != EncryptionInfo::Method::NONE;
}

bool UrlContentToAttachmentConverter::download(
    const std::string& url,
    const std::vector<std::string>& headers,
    ByteVector* content,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher) {
    auto stream = std::make_shared<InProcessAttachment>("download:" + url);
    std::shared_ptr<AttachmentWriter> streamWriter = stream->createWriter(WriterPolicy::BLOCKING);
    if (!download(url, headers, streamWriter, contentFetcher)) {
        ACSDK_ERROR(LX("downloadFailed").d("reason", "downloadToStreamFailed"));
        return false;
    }
    streamWriter->close();

    auto reader = stream->createReader(ReaderPolicy::BLOCKING);
    if (!readContent(std::move(reader), content)) {
        ACSDK_ERROR(LX("downloadFailed").d("reason", "readContentFailed"));
        return false;
    }
    return true;
}

bool UrlContentToAttachmentConverter::readContent(std::shared_ptr<AttachmentReader> reader, ByteVector* content) {
    if (!content) {
        ACSDK_ERROR(LX("readContentFailed").d("reason", "nullContent"));
        return false;
    }

    if (!reader) {
        ACSDK_ERROR(LX("downloadFailed").d("reason", "nullReader"));
        return false;
    }

    auto readStatus = AttachmentReader::ReadStatus::OK;
    ByteVector contentRead;
    ByteVector buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    while (!streamClosed && !m_shuttingDown) {
        auto bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        switch (readStatus) {
            case AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (0 == bytesRead) {
                    break;
                }
                /* FALL THROUGH - to add any data received even if closed */
            case AttachmentReader::ReadStatus::OK:
            case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case AttachmentReader::ReadStatus::OK_TIMEDOUT:
                contentRead.insert(std::end(contentRead), std::begin(buffer), std::begin(buffer) + bytesRead);
                break;
            case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
                // Current AttachmentReader policy renders this outcome impossible.
                ACSDK_ERROR(LX("readContentFailed").d("reason", "overrunReset"));
                break;
            case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("readContentFailed").d("reason", "readError"));
                return false;
        }
    }
    *content = contentRead;
    return true;
}

bool UrlContentToAttachmentConverter::download(
    const std::string& url,
    const std::vector<std::string>& headers,
    std::shared_ptr<AttachmentWriter> streamWriter,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher) {
    if (!streamWriter) {
        ACSDK_ERROR(LX("downloadFailed").d("reason", "nullStreamWriter"));
        return false;
    }

    std::shared_ptr<HTTPContentFetcherInterface> localContentFetcher;

    if (contentFetcher) {
        ACSDK_DEBUG9(LX("usingExistingContentFetcher"));
        localContentFetcher = contentFetcher;
    } else {
        ACSDK_DEBUG9(LX("usingNewContentFetcher"));

        localContentFetcher = m_contentFetcherFactory->create(url);

        localContentFetcher->getContent(HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);

        HTTPContentFetcherInterface::Header header = localContentFetcher->getHeader(&m_shuttingDown);

        if (!header.successful) {
            return false;
        }
    }

    localContentFetcher->getBody(streamWriter);

    while (localContentFetcher->getState() == HTTPContentFetcherInterface::State::FETCHING_BODY) {
        if (m_shuttingDown) {
            ACSDK_DEBUG9(LX("writeDecryptedUrlContentIntoStream").d("info", "shuttingDown"));
            return true;
        }
        std::this_thread::sleep_for(WAIT_FOR_ACTIVITY_TIMEOUT);
    }

    return true;
}

void UrlContentToAttachmentConverter::doShutdown() {
    ACSDK_DEBUG9(LX(__func__).m("Starting to shutdown"));
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_observer.reset();
    }
    m_shuttingDown = true;
    m_contentDecrypter->shutdown();
    m_executor.shutdown();
    m_contentDecrypter.reset();
    m_playlistParser->shutdown();
    m_playlistParser.reset();
    m_streamWriter->close();
    m_streamWriter.reset();
    if (!m_startedStreaming) {
        m_startStreamingPointPromise.set_value(std::chrono::seconds::zero());
    }
}

}  // namespace playlistParser
}  // namespace alexaClientSDK

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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "AVSCommon/Utils/HTTP/HttpResponseCode.h"
#include "AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

using namespace avsCommon::utils::http;

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2MimeResponseDecoder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// ASCII value of CR
static const char CARRIAGE_RETURN_ASCII = 13;
/// ASCII value of LF
static const char LINE_FEED_ASCII = 10;
/// CRLF sequence
static const char CRLF_SEQUENCE[] = {CARRIAGE_RETURN_ASCII, LINE_FEED_ASCII};
/// Size of CLRF in chars
static const int LEADING_CRLF_CHAR_SIZE = sizeof(CRLF_SEQUENCE) / sizeof(*CRLF_SEQUENCE);

/// MIME boundary string prefix in HTTP header.
static const std::string BOUNDARY_PREFIX = "boundary=";
/// Size in chars of the MIME boundary string prefix
static const int BOUNDARY_PREFIX_SIZE = BOUNDARY_PREFIX.size();
/// MIME HTTP header value delimiter
static const std::string BOUNDARY_DELIMITER = ";";

HTTP2MimeResponseDecoder::HTTP2MimeResponseDecoder(std::shared_ptr<HTTP2MimeResponseSinkInterface> sink) :
        m_sink{sink},
        m_responseCode{0},
        m_lastStatus{HTTP2ReceiveDataStatus::SUCCESS},
        m_index{0},
        m_leadingCRLFCharsLeftToRemove{LEADING_CRLF_CHAR_SIZE},
        m_boundaryFound{false},
        m_lastSuccessIndex{0} {
    m_multipartReader.onPartBegin = HTTP2MimeResponseDecoder::partBeginCallback;
    m_multipartReader.onPartData = HTTP2MimeResponseDecoder::partDataCallback;
    m_multipartReader.onPartEnd = HTTP2MimeResponseDecoder::partEndCallback;
    m_multipartReader.userData = this;
    ACSDK_DEBUG5(LX(__func__));
}

bool HTTP2MimeResponseDecoder::onReceiveResponseCode(long responseCode) {
    ACSDK_DEBUG5(LX(__func__).d("responseCode", responseCode));
    m_responseCode = responseCode;

    if (!m_sink) {
        ACSDK_WARN(LX("onReceiveResponseCodeIgnored").d("reason", "nullSink"));
        return false;
    }

    return m_sink->onReceiveResponseCode(m_responseCode);
}

bool HTTP2MimeResponseDecoder::onReceiveHeaderLine(const std::string& line) {
    ACSDK_DEBUG5(LX(__func__).d("line", line));

    if (!m_sink) {
        ACSDK_WARN(LX("onReceiveHeaderLineIgnored").d("reason", "nullSink"));
        return false;
    }

    if (!m_boundaryFound) {
        if (line.find(BOUNDARY_PREFIX) != std::string::npos) {
            std::string boundary{line.substr(line.find(BOUNDARY_PREFIX))};
            boundary = boundary.substr(BOUNDARY_PREFIX_SIZE, boundary.find(BOUNDARY_DELIMITER) - BOUNDARY_PREFIX_SIZE);
            m_multipartReader.setBoundary(boundary);
            m_boundaryFound = true;
        }
    }

    return m_sink->onReceiveHeaderLine(line);
}

void HTTP2MimeResponseDecoder::partBeginCallback(const MultipartHeaders& headers, void* userData) {
    HTTP2MimeResponseDecoder* decoder = static_cast<HTTP2MimeResponseDecoder*>(userData);
    if (!decoder) {
        ACSDK_ERROR(LX("partBeginCallbackFailed").d("reason", "nullDecoder"));
        return;
    }
    switch (decoder->m_lastStatus) {
        case HTTP2ReceiveDataStatus::SUCCESS:
            // Pass notification and headers through to our sink.
            if (!decoder->m_sink->onBeginMimePart(headers)) {
                // Sink doesn't want the next part? ABORT!
                decoder->m_lastStatus = HTTP2ReceiveDataStatus::ABORT;
            }
        case HTTP2ReceiveDataStatus::PAUSE:
            // PAUSE state will switch to SUCCESS when partDataCallback() consumes the previously paused
            // part chunk.  So, ignore any previous parsed part-begin notifications or notifications
            // that arrive after our sink switches to the PAUSED state.
            break;
        case HTTP2ReceiveDataStatus::ABORT:
            // If in the ABORT state, ignore all notifications.
            break;
    }
}

void HTTP2MimeResponseDecoder::partDataCallback(const char* buffer, size_t size, void* userData) {
    HTTP2MimeResponseDecoder* decoder = static_cast<HTTP2MimeResponseDecoder*>(userData);
    if (!decoder) {
        ACSDK_ERROR(LX("partDataCallbackFailed").d("reason", "nullDecoder"));
        return;
    }
    if (!buffer) {
        ACSDK_ERROR(LX("partDataCallbackFailed").d("reason", "nullBuffer"));
        return;
    }
    // Increment the counter for partDataCallbacks.
    decoder->m_index++;
    // Check if we can resume callbacks to sink again.
    if (decoder->m_lastStatus == HTTP2ReceiveDataStatus::PAUSE &&
        decoder->m_index == (decoder->m_lastSuccessIndex + 1)) {
        decoder->m_lastStatus = HTTP2ReceiveDataStatus::SUCCESS;
    }
    // Continue only if successful so far.
    if (decoder->m_lastStatus == HTTP2ReceiveDataStatus::SUCCESS) {
        decoder->m_lastStatus = decoder->m_sink->onReceiveMimeData(buffer, size);
        // Update checkpoint.
        if (decoder->m_lastStatus == HTTP2ReceiveDataStatus::SUCCESS) {
            decoder->m_lastSuccessIndex = decoder->m_index;
        }
    }
}

void HTTP2MimeResponseDecoder::partEndCallback(void* userData) {
    HTTP2MimeResponseDecoder* decoder = static_cast<HTTP2MimeResponseDecoder*>(userData);
    if (!decoder) {
        ACSDK_ERROR(LX("partEndCallbackFailed").d("reason", "nullDecoder"));
        return;
    }
    if ((decoder->m_lastStatus == HTTP2ReceiveDataStatus::SUCCESS) && !decoder->m_sink->onEndMimePart()) {
        decoder->m_lastStatus = HTTP2ReceiveDataStatus::ABORT;
    }
}

HTTP2ReceiveDataStatus HTTP2MimeResponseDecoder::onReceiveData(const char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size));
    if (!bytes) {
        ACSDK_ERROR(LX("onReceivedDataFailed").d("reason", "nullBytes"));
        return HTTP2ReceiveDataStatus::ABORT;
    }
    if (m_lastStatus != HTTP2ReceiveDataStatus::ABORT) {
        // If a non successful response code was received, don't parse the data. Instead forward it to our sink.
        if (m_responseCode != HTTPResponseCode::SUCCESS_OK) {
            if (m_sink) {
                return m_sink->onReceiveNonMimeData(bytes, size);
            } else {
                return HTTP2ReceiveDataStatus::ABORT;
            }
        }

        auto readerCheckpoint = m_multipartReader;
        auto oldLeadingCRLFCharsLeftToRemove = m_leadingCRLFCharsLeftToRemove;
        /**
         * If no boundary found...
         */
        if (!m_boundaryFound) {
            return HTTP2ReceiveDataStatus::ABORT;
        }

        /**
         * Our parser expects no leading CRLF in the data stream. Additionally downchannel streams
         * include this CRLF but event streams do not. So ensure that a leading CRLF in the data stream is removed,
         * if it exists.
         */
        if (m_leadingCRLFCharsLeftToRemove > 0) {
            size_t posInCRLF = LEADING_CRLF_CHAR_SIZE - m_leadingCRLFCharsLeftToRemove;
            while (m_leadingCRLFCharsLeftToRemove > 0 && size > 0) {
                if (*bytes == CRLF_SEQUENCE[posInCRLF]) {
                    bytes++;
                    size--;
                    m_leadingCRLFCharsLeftToRemove--;
                    posInCRLF++;
                } else {
                    if (1 == m_leadingCRLFCharsLeftToRemove) {
                        // Having only CR instead of CRLF indicates a broken stream, so we may safely report error here.
                        return HTTP2ReceiveDataStatus::ABORT;
                    }
                    m_leadingCRLFCharsLeftToRemove = 0;
                    break;
                }
            }

            if (0 == size) {
                return HTTP2ReceiveDataStatus::SUCCESS;
            }
        }

        m_index = 0;
        m_multipartReader.feed(bytes, size);

        if (m_multipartReader.hasError()) {
            ACSDK_ERROR(LX("onReceiveDataFailed")
                            .d("reason", "mimeParseError")
                            .d("error", m_multipartReader.getErrorMessage()));
            m_lastStatus = HTTP2ReceiveDataStatus::ABORT;
        }

        switch (m_lastStatus) {
            case HTTP2ReceiveDataStatus::SUCCESS:
                m_lastSuccessIndex = 0;
                break;
            case HTTP2ReceiveDataStatus::PAUSE:
                m_multipartReader = readerCheckpoint;
                m_leadingCRLFCharsLeftToRemove = oldLeadingCRLFCharsLeftToRemove;
                break;
            case HTTP2ReceiveDataStatus::ABORT:
                break;
        }
    }

    return m_lastStatus;
}

void HTTP2MimeResponseDecoder::onResponseFinished(HTTP2ResponseFinishedStatus status) {
    ACSDK_DEBUG5(LX(__func__).d("status", status));

    if (!m_sink) {
        ACSDK_WARN(LX("onResponseFinishedIgnored").d("reason", "nullSink"));
        return;
    }

    m_sink->onResponseFinished(status);
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

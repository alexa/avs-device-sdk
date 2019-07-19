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

#include <algorithm>
#include <iostream>
#include <set>
#include <utility>
#include <vector>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "AVSCommon/Utils/HTTP2/HTTP2MimeRequestEncoder.h"
#include "AVSCommon/Utils/HTTP2/HTTP2MimeRequestSourceInterface.h"

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2MimeRequestEncoder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

static const std::string BOUNDARY_HEADER_PREFIX = "Content-Type: multipart/form-data; boundary=";

static const std::string CRLF = "\r\n";

static const std::string TWO_DASHES = "--";

std::ostream& operator<<(std::ostream& stream, HTTP2MimeRequestEncoder::State state) {
    switch (state) {
        case HTTP2MimeRequestEncoder::State::NEW:
            return stream << "NEW";
        case HTTP2MimeRequestEncoder::State::GETTING_1ST_PART_HEADERS:
            return stream << "GETTING_1ST_PART_HEADERS";
        case HTTP2MimeRequestEncoder::State::SENDING_1ST_BOUNDARY:
            return stream << "SENDING_1ST_BOUNDARY";
        case HTTP2MimeRequestEncoder::State::SENDING_PART_HEADERS:
            return stream << "SENDING_PART_HEADERS";
        case HTTP2MimeRequestEncoder::State::SENDING_PART_DATA:
            return stream << "SENDING_PART_DATA";
        case HTTP2MimeRequestEncoder::State::SENDING_END_BOUNDARY:
            return stream << "SENDING_END_BOUNDARY";
        case HTTP2MimeRequestEncoder::State::GETTING_NTH_PART_HEADERS:
            return stream << "GETTING_NTH_PART_HEADERS";
        case HTTP2MimeRequestEncoder::State::SENDING_CRLF_AFTER_BOUNDARY:
            return stream << "SENDING_CRLF_AFTER_BOUNDARY";
        case HTTP2MimeRequestEncoder::State::SENDING_TERMINATING_DASHES:
            return stream << "SENDING_TERMINATING_DASHES";
        case HTTP2MimeRequestEncoder::State::DONE:
            return stream << "DONE";
        case HTTP2MimeRequestEncoder::State::ABORT:
            return stream << "ABORT";
    }
    return stream << "UNKNOWN(" << static_cast<int>(state) << ")";
}

HTTP2MimeRequestEncoder::HTTP2MimeRequestEncoder(
    const std::string& boundary,
    std::shared_ptr<HTTP2MimeRequestSourceInterface> source) :
        m_state{State::NEW},
        m_rawBoundary{boundary},
        m_prefixedBoundary{CRLF + TWO_DASHES + boundary},
        m_source{source},
        m_getMimeHeaderLinesResult{HTTP2GetMimeHeadersResult::ABORT},
        m_stringIndex{0} {
    ACSDK_DEBUG5(LX(__func__).d("boundary", boundary).d("source", source.get()));
}

HTTP2SendDataResult HTTP2MimeRequestEncoder::onSendData(char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size).d("state", m_state));

    if (!m_source) {
        return HTTP2SendDataResult::COMPLETE;
    }

    m_bytesCopied = 0;

    while (true) {
        switch (m_state) {
            case State::NEW: {
                setState(State::GETTING_1ST_PART_HEADERS);
            } break;

            case State::GETTING_1ST_PART_HEADERS:
                m_getMimeHeaderLinesResult = m_source->getMimePartHeaderLines();
                switch (m_getMimeHeaderLinesResult.status) {
                    case HTTP2SendStatus::CONTINUE:
                        m_stringIndex = 0;
                        setState(State::SENDING_1ST_BOUNDARY);
                        break;

                    case HTTP2SendStatus::PAUSE:
                        if (m_bytesCopied != 0) {
                            return continueResult();
                        }
                        return HTTP2SendDataResult::PAUSE;

                    case HTTP2SendStatus::COMPLETE:
                        setState(State::DONE);
                        return continueResult();

                    case HTTP2SendStatus::ABORT: {
                        setState(State::ABORT);
                        return HTTP2SendDataResult::ABORT;
                    }
                }
                break;

            case State::SENDING_1ST_BOUNDARY:
                if (!sendStringAndCRLF(bytes, size, m_prefixedBoundary)) {
                    return continueResult();
                }
                m_headerLine = m_getMimeHeaderLinesResult.headers.begin();
                m_stringIndex = 0;
                setState(State::SENDING_PART_HEADERS);
                break;

            case State::SENDING_PART_HEADERS:
                while (m_headerLine != m_getMimeHeaderLinesResult.headers.end()) {
                    if (sendStringAndCRLF(bytes, size, *m_headerLine)) {
                        m_headerLine++;
                        m_stringIndex = 0;
                    } else {
                        return continueResult();
                    }
                }
                if (sendString(bytes, size, CRLF)) {
                    setState(State::SENDING_PART_DATA);
                } else {
                    return continueResult();
                }
                break;

            case State::SENDING_PART_DATA: {
                auto sendPartResult = m_source->onSendMimePartData(bytes + m_bytesCopied, size - m_bytesCopied);
                switch (sendPartResult.status) {
                    case HTTP2SendStatus::CONTINUE:
                        m_bytesCopied += sendPartResult.size;
                        if (m_bytesCopied == size) {
                            return continueResult();
                        } else if (m_bytesCopied > size) {
                            setState(State::ABORT);
                            return HTTP2SendDataResult::ABORT;
                        }
                        // Continues back around to case State::SENDING_PART_DATA.
                        break;

                    case HTTP2SendStatus::PAUSE:
                        if (m_bytesCopied != 0) {
                            return continueResult();
                        }
                        return HTTP2SendDataResult::PAUSE;

                    case HTTP2SendStatus::COMPLETE:
                        m_stringIndex = 0;
                        setState(State::SENDING_END_BOUNDARY);
                        break;

                    case HTTP2SendStatus::ABORT:
                        setState(State::ABORT);
                        return HTTP2SendDataResult::ABORT;
                }
            } break;

            case State::SENDING_END_BOUNDARY:
                if (!sendString(bytes, size, m_prefixedBoundary)) {
                    return continueResult();
                }
                setState(State::GETTING_NTH_PART_HEADERS);
                break;

            case State::GETTING_NTH_PART_HEADERS:
                m_getMimeHeaderLinesResult = m_source->getMimePartHeaderLines();
                switch (m_getMimeHeaderLinesResult.status) {
                    case HTTP2SendStatus::CONTINUE:
                        m_stringIndex = 0;
                        setState(State::SENDING_CRLF_AFTER_BOUNDARY);
                        break;

                    case HTTP2SendStatus::PAUSE:
                        if (m_bytesCopied != 0) {
                            return continueResult();
                        }
                        return HTTP2SendDataResult::PAUSE;

                    case HTTP2SendStatus::COMPLETE:
                        m_stringIndex = 0;
                        setState(State::SENDING_TERMINATING_DASHES);
                        break;

                    case HTTP2SendStatus::ABORT:
                        setState(State::ABORT);
                        return HTTP2SendDataResult::ABORT;
                }
                break;

            case State::SENDING_CRLF_AFTER_BOUNDARY:
                if (!sendString(bytes, size, CRLF)) {
                    return continueResult();
                }
                m_headerLine = m_getMimeHeaderLinesResult.headers.begin();
                m_stringIndex = 0;
                setState(State::SENDING_PART_HEADERS);
                break;

            case State::SENDING_TERMINATING_DASHES:
                if (sendStringAndCRLF(bytes, size, TWO_DASHES)) {
                    setState(State::DONE);
                }
                return continueResult();
                break;

            case State::DONE:
                return HTTP2SendDataResult::COMPLETE;
                break;

            case State::ABORT:
                return HTTP2SendDataResult::ABORT;
        }
    }
}

std::vector<std::string> HTTP2MimeRequestEncoder::getRequestHeaderLines() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_source) {
        auto lines = m_source->getRequestHeaderLines();
        lines.push_back(BOUNDARY_HEADER_PREFIX + m_rawBoundary);
        return lines;
    }
    return {};
}

void HTTP2MimeRequestEncoder::setState(State newState) {
    if (newState == m_state) {
        ACSDK_DEBUG9(LX("nonStateChangeInSetState").d("state", m_state).d("newState", newState));
        return;
    }

    static const std::set<std::pair<State, State>> transitions = {
        {State::NEW, State::GETTING_1ST_PART_HEADERS},
        {State::GETTING_1ST_PART_HEADERS, State::SENDING_1ST_BOUNDARY},
        {State::GETTING_1ST_PART_HEADERS, State::DONE},
        {State::GETTING_1ST_PART_HEADERS, State::ABORT},
        {State::SENDING_1ST_BOUNDARY, State::SENDING_PART_HEADERS},
        {State::SENDING_PART_HEADERS, State::SENDING_PART_DATA},
        {State::SENDING_PART_DATA, State::SENDING_END_BOUNDARY},
        {State::SENDING_PART_DATA, State::DONE},
        {State::SENDING_PART_DATA, State::ABORT},
        {State::SENDING_END_BOUNDARY, State::GETTING_NTH_PART_HEADERS},
        {State::GETTING_NTH_PART_HEADERS, State::SENDING_CRLF_AFTER_BOUNDARY},
        {State::GETTING_NTH_PART_HEADERS, State::SENDING_TERMINATING_DASHES},
        {State::GETTING_NTH_PART_HEADERS, State::ABORT},
        {State::SENDING_CRLF_AFTER_BOUNDARY, State::SENDING_PART_HEADERS},
        {State::SENDING_TERMINATING_DASHES, State::DONE},
    };

    if (transitions.find({m_state, newState}) != transitions.end()) {
        ACSDK_DEBUG9(LX("setState").d("state", m_state).d("newState", newState));
        m_state = newState;
    } else {
        ACSDK_DEBUG9(LX("setStateFailed").d("reason", "noAllowed").d("state", m_state).d("newState", newState));
        m_state = State::DONE;
    }
}

bool HTTP2MimeRequestEncoder::sendString(char* bytes, size_t size, const std::string& text) {
    auto bufferRemaining = size - m_bytesCopied;
    auto textRemaining = text.size() - m_stringIndex;
    auto result = bufferRemaining > textRemaining;
    auto sizeToCopy = result ? textRemaining : bufferRemaining;
    std::memcpy(bytes + m_bytesCopied, text.c_str() + m_stringIndex, sizeToCopy);
    m_bytesCopied += sizeToCopy;
    m_stringIndex += sizeToCopy;
    return result;
}

bool HTTP2MimeRequestEncoder::sendStringAndCRLF(char* bytes, size_t size, const std::string& text) {
    if (m_stringIndex < text.size()) {
        if (!sendString(bytes, size, text)) {
            return false;
        }
    }

    auto crlfBytesCopied = m_stringIndex - text.size();
    auto bufferRemaining = size - m_bytesCopied;
    auto crlfRemaining = CRLF.size() - crlfBytesCopied;
    auto result = bufferRemaining > crlfRemaining;
    auto sizeToCopy = result ? crlfRemaining : bufferRemaining;
    std::memcpy(bytes + m_bytesCopied, CRLF.c_str() + crlfBytesCopied, sizeToCopy);
    m_bytesCopied += sizeToCopy;
    m_stringIndex += sizeToCopy;
    return result;
}

HTTP2SendDataResult HTTP2MimeRequestEncoder::continueResult() {
    return HTTP2SendDataResult(m_bytesCopied);
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

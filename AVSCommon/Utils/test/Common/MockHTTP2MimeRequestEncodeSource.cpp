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

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "AVSCommon/Utils/HTTP2/MockHTTP2MimeRequestEncodeSource.h"
#include "AVSCommon/Utils/Common/Common.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/// PAUSE probability set to 25% after first mandatory PAUSE
#define SHOULD_PAUSE (generateRandomNumber(1, 20) < 5 || m_pauseCount == 0)

HTTP2GetMimeHeadersResult MockHTTP2MimeRequestEncodeSource::getMimePartHeaderLines() {
    if (m_abort) {
        return HTTP2GetMimeHeadersResult::ABORT;
    } else if (m_slowSource && SHOULD_PAUSE) {
        // return PAUSE probabilistically
        m_pauseCount++;
        return HTTP2GetMimeHeadersResult::PAUSE;
    }
    if (m_index < m_headers.size()) {
        return HTTP2GetMimeHeadersResult(m_headers[m_index]);
    }
    return HTTP2GetMimeHeadersResult::COMPLETE;
}

HTTP2SendDataResult MockHTTP2MimeRequestEncodeSource::onSendMimePartData(char* bytes, size_t size) {
    if (m_abort) {
        return HTTP2SendDataResult::ABORT;
    } else if (m_slowSource && SHOULD_PAUSE) {
        m_pauseCount++;
        return HTTP2SendDataResult::PAUSE;
    }
    const char* testPayload = m_data[m_index].c_str();
    size_t bytesRemaining = strlen(testPayload) - m_bytesWritten;

    if (bytesRemaining == 0) {
        m_index++;
        m_bytesWritten = 0;
        return HTTP2SendDataResult::COMPLETE;
    }

    size_t bytesToWrite = size >= bytesRemaining ? bytesRemaining : size;
    std::strncpy(bytes, testPayload + m_bytesWritten, bytesToWrite);
    m_bytesWritten += bytesToWrite;
    return HTTP2SendDataResult(bytesToWrite);
}

std::vector<std::string> MockHTTP2MimeRequestEncodeSource::getRequestHeaderLines() {
    return std::vector<std::string>();
}

MockHTTP2MimeRequestEncodeSource::MockHTTP2MimeRequestEncodeSource(
    const std::vector<std::string>& m_data,
    const std::vector<std::vector<std::string>>& m_headers) :
        m_data{m_data},
        m_headers{m_headers},
        m_bytesWritten{0},
        m_index{0},
        m_slowSource{false},
        m_abort{false},
        m_pauseCount{0} {
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

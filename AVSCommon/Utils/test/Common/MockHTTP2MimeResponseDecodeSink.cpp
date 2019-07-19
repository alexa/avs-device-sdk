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
#include <algorithm>
#include "AVSCommon/Utils/HTTP2/MockHTTP2MimeResponseDecodeSink.h"
#include "AVSCommon/Utils/Common/Common.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/// PAUSE probability set to 25% after first mandatory pause
#define SHOULD_PAUSE (generateRandomNumber(1, 20) < 5 || m_pauseCount == 0)

MockHTTP2MimeResponseDecodeSink::MockHTTP2MimeResponseDecodeSink() :
        m_index{0},
        m_slowSink{false},
        m_abort{false},
        m_pauseCount{0} {
}

bool MockHTTP2MimeResponseDecodeSink::onReceiveResponseCode(long responseCode) {
    return true;
}

bool MockHTTP2MimeResponseDecodeSink::onReceiveHeaderLine(const std::string& line) {
    return true;
}

bool MockHTTP2MimeResponseDecodeSink::onBeginMimePart(const std::multimap<std::string, std::string>& headers) {
    if (m_abort) {
        return false;
    }
    m_data.push_back("");
    m_headers.push_back(headers);
    return true;
}

bool MockHTTP2MimeResponseDecodeSink::onEndMimePart() {
    if (m_abort) {
        return false;
    }
    m_index++;
    return true;
}

HTTP2ReceiveDataStatus MockHTTP2MimeResponseDecodeSink::onReceiveMimeData(const char* bytes, size_t size) {
    if (m_abort) {
        return HTTP2ReceiveDataStatus::ABORT;
    } else if (m_slowSink && SHOULD_PAUSE) {
        m_pauseCount++;
        return HTTP2ReceiveDataStatus::PAUSE;
    }
    char temp[size + 1];
    strncpy(temp, bytes, size);
    temp[size] = '\0';
    m_data.at(m_index) += temp;
    return HTTP2ReceiveDataStatus::SUCCESS;
}

HTTP2ReceiveDataStatus MockHTTP2MimeResponseDecodeSink::onReceiveNonMimeData(const char* bytes, size_t size) {
    m_nonMimeData.append(bytes, size);
    return HTTP2ReceiveDataStatus::SUCCESS;
}

void MockHTTP2MimeResponseDecodeSink::onResponseFinished(HTTP2ResponseFinishedStatus status) {
}

bool MockHTTP2MimeResponseDecodeSink::hasSameContentAs(std::shared_ptr<MockHTTP2MimeRequestEncodeSource> source) {
    bool result{true};
    result &= (source->m_data.size() == m_data.size());
    size_t index{0};
    for (auto part : source->m_data) {
        result &= (part == m_data.at(index));
        index++;
    }
    index = 0;
    for (auto headers : m_headers) {
        for (auto header : headers) {
            auto sourceHeaders = source->m_headers.at(index);
            result &=
                (std::find(sourceHeaders.begin(), sourceHeaders.end(), header.first + ": " + header.second) !=
                 sourceHeaders.end());
        }
        index++;
    }
    return result;
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

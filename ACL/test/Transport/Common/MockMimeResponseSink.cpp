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

#include "MockMimeResponseSink.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

MockMimeResponseSink::MockMimeResponseSink(){

};

bool MockMimeResponseSink::onReceiveResponseCode(long responseCode) {
    return true;
}

bool MockMimeResponseSink::onReceiveHeaderLine(const std::string& line) {
    return true;
}

bool MockMimeResponseSink::onBeginMimePart(const std::multimap<std::string, std::string>& headers) {
    m_mimeCurrentContent.clear();
    return true;
}

avsCommon::utils::http2::HTTP2ReceiveDataStatus MockMimeResponseSink::onReceiveMimeData(
    const char* bytes,
    size_t size) {
    for (unsigned i = 0; i < size; i++) {
        m_mimeCurrentContent.push_back(bytes[i]);
    }
    return avsCommon::utils::http2::HTTP2ReceiveDataStatus::SUCCESS;
}

bool MockMimeResponseSink::onEndMimePart() {
    m_mimeContents.push_back(m_mimeCurrentContent);
    return true;
}

avsCommon::utils::http2::HTTP2ReceiveDataStatus MockMimeResponseSink::onReceiveNonMimeData(
    const char* bytes,
    size_t size) {
    return avsCommon::utils::http2::HTTP2ReceiveDataStatus::SUCCESS;
}

void MockMimeResponseSink::onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status) {
}

std::vector<char> MockMimeResponseSink::getMimePart(unsigned part) {
    return m_mimeContents[part];
}

unsigned MockMimeResponseSink::getCountOfMimeParts() {
    return m_mimeContents.size();
}

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK
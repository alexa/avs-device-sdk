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

#include "MockHTTP2Request.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {
namespace test {

MockHTTP2Request::MockHTTP2Request(const alexaClientSDK::avsCommon::utils::http2::HTTP2RequestConfig& config) :
        m_url{config.getUrl()},
        m_source{config.getSource()},
        m_sink{config.getSink()},
        m_type{config.getRequestType()} {
    m_mimeResponseSink = std::make_shared<acl::test::MockMimeResponseSink>();
    m_mimeDecoder = std::make_shared<HTTP2MimeResponseDecoder>(m_mimeResponseSink);
}

const std::string MockHTTP2Request::getUrl() {
    return m_url;
}

std::shared_ptr<HTTP2RequestSourceInterface> MockHTTP2Request::getSource() {
    return m_source;
}

std::shared_ptr<HTTP2ResponseSinkInterface> MockHTTP2Request::getSink() {
    return m_sink;
}

HTTP2RequestType MockHTTP2Request::getRequestType() {
    return m_type;
}

std::shared_ptr<acl::test::MockMimeResponseSink> MockHTTP2Request::getMimeResponseSink() {
    return m_mimeResponseSink;
}

std::shared_ptr<HTTP2MimeResponseDecoder> MockHTTP2Request::getMimeDecoder() {
    return m_mimeDecoder;
}

}  // namespace test
}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

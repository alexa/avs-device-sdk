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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKHTTP2REQUEST_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKHTTP2REQUEST_H_

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestConfig.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestInterface.h>

#include "MockMimeResponseSink.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {
namespace test {

/**
 * A mock class of @c HTTP2RequestInterface.
 */
class MockHTTP2Request : public HTTP2RequestInterface {
public:
    MockHTTP2Request(const alexaClientSDK::avsCommon::utils::http2::HTTP2RequestConfig& config);
    MOCK_METHOD0(cancel, bool());
    MOCK_CONST_METHOD0(getId, std::string());

    /**
     * Retrieve the URL for the request.
     *
     * @return The request URL.
     */
    const std::string getUrl();

    /**
     * Retrieve the HTTP2 request source.
     *
     * @return The HTTP2 request source.
     */
    std::shared_ptr<HTTP2RequestSourceInterface> getSource();

    /**
     * Retrieve the HTTP2 request sink.
     *
     * @return The HTTP2 request sink.
     */
    std::shared_ptr<HTTP2ResponseSinkInterface> getSink();

    /**
     * Retrieve the HTTP2 request type.
     *
     * @return The HTTP2 request type.
     */
    HTTP2RequestType getRequestType();

    /**
     * Retrieve The MIME response sink that contains the parsed mime contents of the HTTP2 request.
     *
     * @return The pointer to the MIME response sink.
     */
    std::shared_ptr<acl::test::MockMimeResponseSink> getMimeResponseSink();

    /**
     * Retrieve The MIME response decoder of the HTTP2 request.
     *
     * @return The pointer to the MIME response decoder.
     */
    std::shared_ptr<HTTP2MimeResponseDecoder> getMimeDecoder();

private:
    /// The URL to receive the request.
    const std::string m_url;

    /// The object to provide the data for this HTTP2 POST request.
    std::shared_ptr<HTTP2RequestSourceInterface> m_source;

    /// The object to receive the response to this HTTP2 request.
    std::shared_ptr<HTTP2ResponseSinkInterface> m_sink;

    /// The HTTP Request Type.
    HTTP2RequestType m_type;

    /// The MIME response sink that contains the parsed mime contents of the HTTP2 request.
    std::shared_ptr<acl::test::MockMimeResponseSink> m_mimeResponseSink;

    /// An instance of the @c HTTP2MimeResponseDecoder which decodes the contents of an HTTP2 request.
    std::shared_ptr<HTTP2MimeResponseDecoder> m_mimeDecoder;
};

}  // namespace test
}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKHTTP2REQUEST_H_ */

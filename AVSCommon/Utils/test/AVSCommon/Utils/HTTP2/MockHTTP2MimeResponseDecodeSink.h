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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_HTTP2_MOCKHTTP2MIMERESPONSEDECODESINK_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_HTTP2_MOCKHTTP2MIMERESPONSEDECODESINK_H_

#include <string>
#include <vector>
#include <memory>

#include "AVSCommon/Utils/HTTP2/HTTP2MimeResponseSinkInterface.h"
#include "AVSCommon/Utils/HTTP2/MockHTTP2MimeRequestEncodeSource.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {
/**
 * Mock class which implements the @class HTTP2MimeResponseSinkInterface
 * to allow testing
 */
class MockHTTP2MimeResponseDecodeSink : public HTTP2MimeResponseSinkInterface {
public:
    /**
     * Constructor to provide the HTTP headers and data
     * to be passed onto the decoder
     */
    MockHTTP2MimeResponseDecodeSink();

    /// @name HTTP2MimeResponseSinkInterface methods.
    /// @{
    bool onReceiveResponseCode(long responseCode) override;
    bool onReceiveHeaderLine(const std::string& line) override;
    bool onBeginMimePart(const std::multimap<std::string, std::string>& headers) override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveMimeData(const char* bytes, size_t size) override;
    bool onEndMimePart() override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveNonMimeData(const char* bytes, size_t size) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status) override;
    /// @}

    /**
     * Helper method to compare data with source
     * @param source RequestSource used to generate the original encoded message
     * @return true if data is same
     */
    bool hasSameContentAs(std::shared_ptr<MockHTTP2MimeRequestEncodeSource> source);

    /**
     * Destructor
     */
    ~MockHTTP2MimeResponseDecodeSink() = default;

    /**
     * These will be kept public to help testing
     */

    /// MIME data parts received
    std::vector<std::string> m_data;
    /// MIME headers received for every part
    std::vector<std::multimap<std::string, std::string>> m_headers;
    /// current MIME part index
    size_t m_index{0};
    /// enable sending PAUSE intermittently
    bool m_slowSink{false};
    /// If ABORT is to be sent
    bool m_abort{false};
    /// PAUSE count
    size_t m_pauseCount{0};
    /// Non mime data received.
    std::string m_nonMimeData;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_HTTP2_MOCKHTTP2MIMERESPONSEDECODESINK_H_

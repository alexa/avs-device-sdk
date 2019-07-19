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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_HTTP2_MOCKHTTP2MIMEREQUESTENCODESOURCE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_HTTP2_MOCKHTTP2MIMEREQUESTENCODESOURCE_H_

#include <string>
#include <vector>
#include "AVSCommon/Utils/HTTP2/HTTP2MimeRequestSourceInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {
/**
 * Mock class which implements the HTTP2MIMERequestSourceInterface
 * to allow testing
 */
class MockHTTP2MimeRequestEncodeSource : public HTTP2MimeRequestSourceInterface {
public:
    /**
     * Constructor which accepts the various MIME parts to be
     * passed onto the encoder as the request source
     * @param m_data vector of MIME data parts
     * @param m_headers vector of MIME headers
     */
    MockHTTP2MimeRequestEncodeSource(
        const std::vector<std::string>& m_data,
        const std::vector<std::vector<std::string>>& m_headers);

    /// @name HTTP2MimeRequestSourceInterface methods.
    /// @{
    HTTP2GetMimeHeadersResult getMimePartHeaderLines() override;
    HTTP2SendDataResult onSendMimePartData(char* bytes, size_t size) override;
    std::vector<std::string> getRequestHeaderLines() override;
    /// @}

    /**
     * Destructor
     */
    ~MockHTTP2MimeRequestEncodeSource() = default;

    /**
     * These will be kept public to help testing
     */

    /// Stores the MIME data parts
    std::vector<std::string> m_data;

    /// Stores the MIME header parts
    std::vector<std::vector<std::string>> m_headers;

    /// Index into the current MIME data part
    size_t m_bytesWritten;

    /// Index of current MIME part being read
    size_t m_index;

    /// Enable sending PAUSE intermittently
    bool m_slowSource;

    /// If ABORT is to be sent
    bool m_abort;

    /// PAUSE count
    size_t m_pauseCount;
};
}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_HTTP2_MOCKHTTP2MIMEREQUESTENCODESOURCE_H_

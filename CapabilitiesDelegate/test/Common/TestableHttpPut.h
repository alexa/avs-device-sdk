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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEHTTPPUT_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEHTTPPUT_H_

#include <string>
#include <vector>

#include <AVSCommon/Utils/LibcurlUtils/HttpPutInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPResponse.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace avsCommon::utils::libcurlUtils;

/**
 * A test HttpPut handle that will take the HTTP request and send back a response that you want to test with.
 */
class TestHttpPut : public HttpPutInterface {
public:
    /// @name HttpPutInterface method overrides.
    /// @{
    HTTPResponse doPut(const std::string& url, const std::vector<std::string>& headers, const std::string& data)
        override;
    /// @}

    /**
     * Constructor
     */
    TestHttpPut();

    /**
     * Sets the HTTP response code to be returned.
     *
     * @param httpResponseCode The HTTP response code to be returned.
     */
    void setResponseCode(long httpResponseCode);

    /**
     * Sets the HTTP response body to be returned.
     *
     * @param httpResponseBody The HTTP response body to be returned.
     */
    void setResponseBody(std::string httpResponseBody);

    /**
     * Gets the HTTP request URL.
     *
     * @return The HTTP request URL.
     */
    std::string getRequestUrl();

    /**
     * Gets the HTTP request data.
     *
     * @return The HTTP request data.
     */
    std::string getRequestData();

    /**
     * Gets the HTTP request headers.
     *
     * @return The HTTP request headers.
     */
    std::vector<std::string> getRequestHeaders();

    /**
     * Resets all the data associated with this test instance.
     */
    void reset();

private:
    /// request URL
    std::string m_requestUrl;

    /// request headers
    std::vector<std::string> m_requestHeaders;

    /// request data
    std::string m_requestData;

    /// HTTP response code
    long m_httpResponseCode;

    /// HTTP response body
    std::string m_httpResponseBody;
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEHTTPPUT_H_ */

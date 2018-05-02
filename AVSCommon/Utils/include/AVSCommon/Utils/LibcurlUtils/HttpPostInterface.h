/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPPOSTINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPPOSTINTERFACE_H_

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "HTTPResponse.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/// Minimal interface for making Http POST requests.
class HttpPostInterface {
public:
    /// Virtual destructor to assure proper cleanup of derived types.
    virtual ~HttpPostInterface() = default;

    /**
     * Perform an HTTP Post request returning the response body as a string. This method blocks for the duration
     * of the request.
     *
     * @param url The URL to send the POST to.
     * @param data The POST data to send in the request.
     * @param timeout The maximum amount of time (in seconds) to wait for the request to complete.
     * @param[out] body A string to receive the body of the request if there is one.
     * @return A HttpStatus indicating the disposition of the Post request.
     */
    virtual long doPost(
        const std::string& url,
        const std::string& data,
        std::chrono::seconds timeout,
        std::string& body) = 0;

    /**
     * Perform an HTTP Post request returning the response body as a string. This method
     * blocks for the duration of the request.
     *
     * @param url The URL to send the POST to.
     * @param headerLines vector of strings to add as header lines.
     * @param data Key, value pairs describing the POST data to send in the request.  This keys and values will
     * be URL encoded by this method.
     * @param timeout The maximum amount of time (in seconds) to wait for the request to complete.
     * @return An object describing the response to the request.
     */
    virtual HTTPResponse doPost(
        const std::string& url,
        const std::vector<std::string> headerLines,
        const std::vector<std::pair<std::string, std::string>>& data,
        std::chrono::seconds timeout) = 0;

    /**
     * Perform an HTTP Post request returning the response body as a string. This method
     * blocks for the duration of the request.
     *
     * @param url The URL to send the POST to.
     * @param headerLines vector of strings to add as header lines.
     * @param data A string containing the POST data to send in the request.
     * @param timeout The maximum amount of time (in seconds) to wait for the request to complete.
     * @return An object describing the response to the request.
     */
    virtual HTTPResponse doPost(
        const std::string& url,
        const std::vector<std::string> headerLines,
        const std::string& data,
        std::chrono::seconds timeout) = 0;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPPOSTINTERFACE_H_

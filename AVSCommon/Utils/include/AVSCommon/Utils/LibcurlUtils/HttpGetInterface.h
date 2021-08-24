/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPGETINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPGETINTERFACE_H_

#include <string>
#include <vector>

#include "HTTPResponse.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/// Minimal interface for making HTTP GET requests.
class HttpGetInterface {
public:
    /// Virtual destructor to assure proper cleanup of derived types.
    virtual ~HttpGetInterface() = default;

    /**
     * Perform an HTTP Get request returning the response body as a string. This method blocks for the duration
     * of the request.
     *
     * @param url The URL to send the GET to.
     * @param headers Collection of strings to add in the header.
     * @return An object describing the response to the GET request.
     */
    virtual HTTPResponse doGet(const std::string& url, const std::vector<std::string>& headers) = 0;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPGETINTERFACE_H_

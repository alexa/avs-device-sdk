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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPPUT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPPUT_H_

#include <memory>
#include <mutex>
#include <string>

#include <curl/curl.h>

#include "AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h"
#include "AVSCommon/Utils/LibcurlUtils/HttpPutInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/// LIBCURL based implementation of HttpPutInterface.
class HttpPut : public HttpPutInterface {
public:
    /**
     * Create a new HttpPut instance, passing ownership of the new instance on to the caller.
     *
     * @return Returns an std::unique_ptr to the new HttpPut instance, or @c nullptr of the operation failed.
     */
    static std::unique_ptr<HttpPut> create();

    /**
     * HttpPut destructor
     */
    ~HttpPut() = default;

    /**
     * Deleted copy constructor.
     *
     * @param rhs The 'right hand side' to not copy.
     */
    HttpPut(const HttpPut& rhs) = delete;

    /**
     * Deleted assignment operator.
     *
     * @param rhs The 'right hand side' to not copy.
     * @return The object assigned to.
     */
    HttpPut& operator=(const HttpPut& rhs) = delete;

    HTTPResponse doPut(const std::string& url, const std::vector<std::string>& headers, const std::string& data)
        override;

private:
    /**
     * Default HttpPut constructor.
     */
    HttpPut() = default;

    /// Mutex to serialize access to @c m_curl.
    std::mutex m_mutex;

    /// CURL handle with which to make requests
    CurlEasyHandleWrapper m_curl;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPPUT_H_

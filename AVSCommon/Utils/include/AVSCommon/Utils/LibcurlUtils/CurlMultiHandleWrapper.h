/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CURLMULTIHANDLEWRAPPER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CURLMULTIHANDLEWRAPPER_H_

#include <chrono>
#include <curl/curl.h>
#include <memory>
#include <unordered_set>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * This class wraps a @c libcurl @c multi @c handle as a C++ object.
 * Wrapping enables:
 * - tracking handles to ensure that all handles are removed before curl_multi_cleanup() is called.
 * - centralized and consistent logging when curl_multi operations fail.
 * - simplified signatures for calling code.
 * - (slightly) improved type safety (so far just time values).
 */
class CurlMultiHandleWrapper {
public:
    /**
     * Create a CurlMultiHandleWrapper.
     *
     * @return The new CurlMultiHandleWrapper instance, or nullptr if the operation fails.
     */
    static std::unique_ptr<CurlMultiHandleWrapper> create();

    /**
     * Destructor.
     */
    ~CurlMultiHandleWrapper();

    /**
     * Get the @c libcurl @c multi @c handle underlying this instance.
     *
     * @return The @c libcurl @c multi @c handle underlying this instance.
     */
    CURLM* getCurlHandle();

    /**
     * Add a @c libcurl @c handle to this instance.
     *
     * @param handle The @c libcurl @c handle to add to this instance.
     * @return @c libcurl code indicating the result of this operation.
     */
    CURLMcode addHandle(CURL* handle);

    /**
     * Remove a @c libcurl @c handle from this instance.
     *
     * @param handle The @c libcurl @c handle to remove from this instance.
     * @return @c libcurl code indicating the result of this operation.
     */
    CURLMcode removeHandle(CURL* handle);

    /**
     * Read and/or write available data for the @c libcurl @c handles added to this @c libcurl @c multi @c handle.
     *
     * @param[out] runningHandles Returns the number of handles for which read/write operations were performed.
     * @return @c libcurl code indicating the result of this operation.
     */
    CURLMcode perform(int* runningHandles);

    /**
     * Wait for actions to perform on the @c libcurl @c handles added to this @c libcurl @c multi @c handle.
     *
     * @param timeout How long to wait for actions to perform.
     * @param[out] countHandlesUpdated The number of handles for which actions are ready to be performed.
     * @return @c libcurl code indicating the result of this operation.
     */
    CURLMcode wait(std::chrono::milliseconds timeout, int* countHandlesUpdated);

    /**
     * Receive the next messages about the @c libcurl @c handles added to this @c libcurl @c multi @c handle.
     *
     * @param[out] messagesInQueue The number of @c libcurl @c handles  for which messages remain.
     * @return The next message or nullptr if there are no more messages.
     */
    CURLMsg* infoRead(int* messagesInQueue);

private:
    /**
     * Constructor.
     *
     * @param handle The @c libcurl @c multi @c handle to wrap.
     */
    CurlMultiHandleWrapper(CURLM* handle);

    /// The wrapped @c libcurl @c handles.
    CURLM* m_handle;

    /// The set of @c libcurl @c handles added to this instance.
    std::unordered_set<CURL*> m_streamHandles;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CURLMULTIHANDLEWRAPPER_H_

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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2REQUEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2REQUEST_H_

#include <atomic>
#include <chrono>
#include <memory>

#include <AVSCommon/Utils/HTTP2/HTTP2RequestConfig.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestInterface.h>

#include "CurlEasyHandleWrapper.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

class LibcurlHTTP2Request : public alexaClientSDK::avsCommon::utils::http2::HTTP2RequestInterface {
public:
    /**
     * Constructor.
     * @param config
     * @param id Name used to identify this request.
     */
    LibcurlHTTP2Request(const alexaClientSDK::avsCommon::utils::http2::HTTP2RequestConfig& config, std::string id = "");

    /// @name HTTP2RequestInterface methods.
    /// @{
    bool cancel() override;
    std::string getId() const override;
    /// @}

    /**
     * Gets the CURL easy handle associated with this stream
     *
     * @returns The associated curl easy handle for the stream
     */
    CURL* getCurlHandle();

    /**
     * Notify the HTTP2ResponseSinkInterface that the transfer is complete with
     * the appropriate HTTP2ResponseFinishedStatus code.
     *
     * @param status The finished status to report.
     */
    void reportCompletion(http2::HTTP2ResponseFinishedStatus status);

    /**
     * If the response code has been received and not previously reported, notify the
     * HTTP2ResponseSinkInterface that the response code has been received.
     */
    void reportResponseCode();

    /**
     * Return whether or not the activity timeout has been reached.
     *
     * @return Whether or not the activity timeout has been reached.
     */
    bool hasProgressTimedOut() const;

    /**
     * Whether this request expects that transfer will happen intermittently.
     *
     * @return Whether this request expects that transfer will happen intermittently.
     */
    bool isIntermittentTransferExpected() const;

    /**
     * Sets the time of last transfer to the current time.
     */
    inline void setTimeOfLastTransfer();

    /**
     * Un-pause read and write for this request.
     */
    void unPause();

    /**
     * Return whether this stream has pending transfers.
     *
     * @return whether this stream has been paused.
     */
    bool isPaused() const;

    /**
     * Return whether this request has been cancelled.
     *
     * @return whether this request has been cancelled.
     */
    bool isCancelled() const;

private:
    /**
     * Callback that gets executed when data is received.
     *
     * @see CurlEasyHandleWrapper::CurlCallback for details
     *
     * @param data Pointer to the received data.
     * @param size Size of data members.
     * @param nmemb Number of data members.
     * @param userData Context passed back from @c libcurl.  Should always be a pointer to @c LibcurlHTTP2Request.
     * @return @c size if the operation was successful and @c size bytes were received, @c CURL_WRITEFUNC_PAUSE
     * if the bytes could not be processed during the call and @c libcurl should call back later to try again,
     * or some other value (e.g. 0), to abort the request.
     */
    static size_t writeCallback(char* data, size_t size, size_t nmemb, void* userData);

    /**
     * Callback that gets executed when HTTP headers are received.
     *
     * @see CurlEasyHandleWrapper::CurlCallback for details
     *
     * @param data Buffer containing header.  Not null terminated.
     * @param size Size of members in buffer.
     * @param nmemb Number of members in buffer.
     * @param userData Context passed back from @c libcurl.  Should always be a pointer to @c LibcurlHTTP2Request.
     * @return @c size if the operation was successful, or any other value to abort the request.
     */
    static size_t headerCallback(char* data, size_t size, size_t nmemb, void* userData);

    /**
     * Callback that gets executed to acquire data to send.
     *
     * @see CurlEasyHandleWrapper::CurlCallback for details
     *
     * @param data Buffer to receive data to send.
     * @param size Size of members in buffer.
     * @param nmemb Number of members in buffer.
     * @param userData  Context passed back from @c libcurl.  Should always be a pointer to @c LibcurlHTTP2Request.
     * @return Zero to indicate the end of the data to send, 1 to size * nmemb to indicate the number of bytes
     * read in to @c data, or CURL_READFUNC_PAUSE to indicate that no data was read but @c libcurl should call
     * back later to try again.
     */
    static size_t readCallback(char* data, size_t size, size_t nmemb, void* userData);

    /**
     * Returns the HTTP response code to this request.
     *
     * @returns The HTTP response code if one has been received, 0 if not, and < 0 if there is an error
     */
    long getResponseCode();

    /// Provides request headers and body
    std::shared_ptr<http2::HTTP2RequestSourceInterface> m_source;

    /// Receives responses
    std::shared_ptr<http2::HTTP2ResponseSinkInterface> m_sink;

    /// Initially false; set to true after the response code has been reported.
    bool m_responseCodeReported;

    /// Max time the stream may make no progress before @c hasProgressTimedOut() returns true.
    std::chrono::milliseconds m_activityTimeout;

    /// Last time something was transferred.
    std::chrono::steady_clock::time_point m_timeOfLastTransfer;

    /// The underlying curl easy handle.
    CurlEasyHandleWrapper m_stream;

    /// Whether this request expects that transfer will happen intermittently.
    /// If true, the transfer thread may be put to sleep even when this request isn't paused.
    bool m_isIntermittentTransferExpected;

    /// Whether this stream has any paused transfers.
    bool m_isPaused;

    /// Whether this request has been cancelled.
    std::atomic_bool m_isCancelled;
};

void LibcurlHTTP2Request::setTimeOfLastTransfer() {
    m_timeOfLastTransfer = std::chrono::steady_clock::now();
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2REQUEST_H_

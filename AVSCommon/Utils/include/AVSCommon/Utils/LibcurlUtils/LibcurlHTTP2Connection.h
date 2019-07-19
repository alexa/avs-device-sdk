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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2CONNECTION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2CONNECTION_H_

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <thread>

#include "AVSCommon/Utils/HTTP2/HTTP2ConnectionInterface.h"
#include "CurlMultiHandleWrapper.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

class LibcurlHTTP2Request;

class LibcurlHTTP2Connection : public avsCommon::utils::http2::HTTP2ConnectionInterface {
public:
    /**
     * Create an @c LibcurlHTTP2Connection.
     *
     * @return The new @c LibcurlHTTP2Connection or nullptr if the operation fails.
     */
    static std::shared_ptr<LibcurlHTTP2Connection> create();

    /**
     * Destructor.
     */
    ~LibcurlHTTP2Connection();

    /// @name HTTP2ConnectionInterface methods.
    /// @{
    std::shared_ptr<avsCommon::utils::http2::HTTP2RequestInterface> createAndSendRequest(
        const http2::HTTP2RequestConfig& config) override;
    void disconnect() override;
    /// @}

protected:
    /**
     * Constructor
     */
    LibcurlHTTP2Connection();

private:
    /**
     * Adds a configured stream into this connection.
     * @param stream Request object with a configured curl handle.
     *
     * @return @c libcurl code indicating the result of this operation.
     */
    bool addStream(std::shared_ptr<LibcurlHTTP2Request> stream);

    /** Set up the multi-handle.  Re-assign a new multi-handle, if m_multi is already non-null.
     *
     * @return true if successful, false if a failure was encountered
     */
    bool createMultiHandle();

    /** Release a stream.
     *
     * @param stream The stream to release.
     * @return Whether the operation was successful.
     */
    bool releaseStream(LibcurlHTTP2Request& stream);

    /**
     * Main network loop. Repeatedly call curl_multi_perform in order to transfer data on the incorporated streams.
     */
    void networkLoop();

    /**
     * Find out whether or not the network loop is stopping.
     *
     * @return Whether or not the network loop is stopping.
     */
    bool isStopping();

    /**
     * Safely set m_isStopping to break out of the network loop.
     */
    void setIsStopping();

    /**
     * Checks if any active streams have finished and reports the response code and completion status for them.
     */
    void cleanupFinishedStreams();

    /** Checks for streams that have been cancelled, removes them, and reports CANCELLED completion status.
     * Checks for streams that have not progressed within their timeout, removes them, and reports the response code
     * (if any) and completion status (TIMEOUT).
     */
    void cleanupCancelledAndStalledStreams();

    /**
     * Determine whether all non-intermittent streams are paused.  An intermittent stream would be a persistent
     * downchannel stream, for instace.
     * @return True if all non-intermittent streams are paused, false otherwise.
     */
    bool areStreamsPaused();

    /**
     * UnPause all the active streams.
     */
    void unPauseActiveStreams();

    /**
     * Cancel a stream and report CANCELLED completion status.
     *
     * @param stream The stream to cancel.
     * @return Whether the operation was successful.
     */
    bool cancelStream(LibcurlHTTP2Request& stream);

    /**
     * Release any streams that are active and report CANCELLED completion status.
     */
    void cancelAllStreams();

    /**
     * Take the next request from the queue;
     *
     * @return next request, or nullPtr if queue is empty or the network loop is stopping.
     */
    std::shared_ptr<LibcurlHTTP2Request> dequeueRequest();

    /**
     * Dequeue next request and add it to the multi-handle
     */
    void processNextRequest();

    /// Main thread for this class.
    std::thread m_networkThread;

    /// Represents a CURL multi handle.  Intended to only be accessed by the network loop thread.
    std::unique_ptr<avsCommon::utils::libcurlUtils::CurlMultiHandleWrapper> m_multi;

    /// Serializes concurrent access to the m_requestQueue and m_isStopping members.
    std::mutex m_mutex;

    /// Used to notify the network loop thread that there is at least one request queued or that the loop
    /// has been instructed to stop.
    std::condition_variable m_cv;

    /// The list of streams that either do not have HTTP response headers, or have outstanding response data.
    /// Only accessed from the network loop thread.
    std::map<CURL*, std::shared_ptr<LibcurlHTTP2Request>> m_activeStreams;

    /// Queue of requests send. Serialized by @c m_mutex.
    std::deque<std::shared_ptr<LibcurlHTTP2Request>> m_requestQueue;

    /// Set to true when we want to exit the network loop.
    bool m_isStopping;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2CONNECTION_H_

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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2CONNECTION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2CONNECTION_H_

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <thread>

#include "AVSCommon/Utils/HTTP2/HTTP2ConnectionInterface.h"
#include "CurlMultiHandleWrapper.h"
#include "LibcurlSetCurlOptionsCallbackInterface.h"

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
     * @param setCurlOptionsCallback The optional @c LibcurlSetCurlOptionsCallbackInterface to set curl
     * options when a new http2 connection is being created.
     * @return The new @c LibcurlHTTP2Connection or nullptr if the operation fails.
     */
    static std::shared_ptr<LibcurlHTTP2Connection> create(
        const std::shared_ptr<LibcurlSetCurlOptionsCallbackInterface>& setCurlOptionsCallback = nullptr);

    /**
     * Destructor.
     */
    ~LibcurlHTTP2Connection();

    /// @name HTTP2ConnectionInterface methods.
    /// @{
    std::shared_ptr<avsCommon::utils::http2::HTTP2RequestInterface> createAndSendRequest(
        const http2::HTTP2RequestConfig& config) override;
    void disconnect() override;
    void addObserver(std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionObserverInterface> observer) override;
    void removeObserver(std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionObserverInterface> observer) override;
    /// @}

protected:
    /**
     * Constructor.
     *
     * @param setCurlOptionsCallback The optional @c LibcurlSetCurlOptionsCallbackInterface to set curl
     * options when a new http2 connection is being created.
     */
    LibcurlHTTP2Connection(
        const std::shared_ptr<LibcurlSetCurlOptionsCallbackInterface>& setCurlOptionsCallback = nullptr);

    /**
     * Alias for c++ ordered map where key is curl handle and value is pointer to LibcurlHTTP2Request.
     */
    using ActiveStreamMap = std::map<CURL*, std::shared_ptr<LibcurlHTTP2Request>>;
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
     * @param[in,out] iterator The iterator to ActiveStreamMap to erase.
     * @return Whether the operation was successful.
     */
    bool releaseStream(ActiveStreamMap::iterator& iterator);

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
     * Cancel an active stream and report CANCELLED completion status.
     *
     * @param[in,out] iterator The iterator to ActiveStreamMap to erase.
     * @return Whether the operation was successful.
     */
    bool cancelActiveStream(ActiveStreamMap::iterator& iterator);

    /**
     * Release any active streams and report CANCELLED completion status.
     */
    void cancelActiveStreams();

    /**
     * Report CANCELLED completion status on any pending streams in the queue.
     */
    void cancelPendingStreams();

    /**
     * Cancels all streams on cleanup.
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

    /**
     * Notify observers that a GOAWAY frame has been received.
     */
    void notifyObserversOfGoawayReceived();

    /// Main thread for this class.
    std::thread m_networkThread;

    /// Represents a CURL multi handle.  Intended to only be accessed by the network loop thread and in @c addStream.
    std::shared_ptr<avsCommon::utils::libcurlUtils::CurlMultiHandleWrapper> m_multi;

    /// Serializes concurrent access to the m_requestQueue and m_isStopping members.
    std::mutex m_mutex;

    /// Used to notify the network loop thread that there is at least one request queued or that the loop
    /// has been instructed to stop.
    std::condition_variable m_cv;

    /// The list of streams that either do not have HTTP response headers, or have outstanding response data.
    /// Only accessed from the network loop thread.
    ActiveStreamMap m_activeStreams;

    /// Queue of requests send. Serialized by @c m_mutex.
    std::deque<std::shared_ptr<LibcurlHTTP2Request>> m_requestQueue;

    /// Mutex for observers.
    std::mutex m_observersMutex;

    /// Observers
    std::unordered_set<std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionObserverInterface>> m_observers;

    /// Set to true when we want to exit the network loop.
    bool m_isStopping;

    /// The @c LibcurlSetCurlOptionsCallbackInterface used for this connection.
    std::shared_ptr<LibcurlSetCurlOptionsCallbackInterface> m_setCurlOptionsCallback;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTP2CONNECTION_H_

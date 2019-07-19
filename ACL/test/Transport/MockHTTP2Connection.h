/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKHTTP2CONNECTION_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKHTTP2CONNECTION_H_

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ACL/Transport/MessageConsumerInterface.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/HTTP2/HTTP2ConnectionInterface.h>

#include "MockHTTP2Request.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {
namespace test {

/**
 * A mock class of @c HTTP2ConnectionInterface.
 */
class MockHTTP2Connection : public HTTP2ConnectionInterface {
public:
    /**
     * Constructor
     * @param dURL The URL for downchannel requests.
     * @param pingURL The URL for ping requests.
     */
    MockHTTP2Connection(std::string dURL, std::string pingURL);

    ~MockHTTP2Connection() = default;

    /// @name HTTP2ConnectionInterface methods
    /// @{
    std::shared_ptr<HTTP2RequestInterface> createAndSendRequest(const HTTP2RequestConfig& config);
    MOCK_METHOD0(disconnect, void());
    /// @}

    /**
     * Check whether there are any HTTP requests sent.
     *
     * @return True if there are no HTTP requests sent by @c HTTP2Transport.
     */
    bool isRequestQueueEmpty();

    /**
     * Wait for an HTTP request to be sent.
     *
     * @param timeout Wait timeout in milliseconds.
     * @param requestNum The number of HTTP2 requests to wait for.
     * @return A pointer to the request if there is one before the timeout occurs, otherwise nullptr
     */
    std::shared_ptr<MockHTTP2Request> waitForRequest(std::chrono::milliseconds timeout, unsigned requestNum = 1);

    /**
     * Pop the latest HTTP2 Request from queue.
     *
     * @return The oldest HTTP2 Request in the queue.
     */
    std::shared_ptr<MockHTTP2Request> dequeRequest();

    /**
     * Set the Header content to be matched by @c waitForRequestWithHeader.
     *
     * @param matchString The contents of the header to match.
     */
    void setWaitRequestHeader(const std::string& matchString);

    /**
     * Wait for a request with a particular Header content.
     *
     * @param timeout The wait timeout in milliseconds
     * @param matchString The contents of the header that it is waiting for
     * @return True if a match occurred before timeout
     */
    bool waitForRequestWithHeader(std::chrono::milliseconds timeout);

    /**
     * Wait for a POST HTTP2request.
     *
     * @param timeout The wait timeout in milliseconds.
     * @return A pointer to the POST request if there is one before the timeout occurs, otherwise nullptr
     */
    std::shared_ptr<MockHTTP2Request> waitForPostRequest(const std::chrono::milliseconds timeout);

    /**
     * Wait for a Ping HTTP2request.
     *
     * @param timeout The wait timeout in milliseconds.
     * @return A pointer to the Ping request if there is one before the timeout occurs, otherwise nullptr
     */
    std::shared_ptr<MockHTTP2Request> waitForPingRequest(const std::chrono::milliseconds timeout);

    /**
     * Respond to downchannel requests with a response code and notify onResponseFinished() if needed.
     *
     * @param responseCode The HTTP response code to reply.
     * @param sendResponseFinished True if needed to notify onResponseFinished(), otherwise false.
     * @param timeout Timeout for wating for downchannel HTTP2 requests.
     * @return True if a response has been sent to the downchannel request before timeout, otherwise false.
     */
    bool respondToDownchannelRequests(
        long responseCode,
        bool sendResponseFinished,
        const std::chrono::milliseconds timeout);

    /**
     * Set the response code for POST requests that will be immediately replied when an HTTP POST request is sent.
     *
     * @param responseCode The HTTP response code to reply to the request. If set to @c
     * HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED, a response code will not be sent.
     */
    void setResponseToPOSTRequests(http::HTTPResponseCode responseCode);

    /**
     * Retrieve the first HTTP2 request made on the downchannel.
     *
     * @return The first downchannel request or nullptr if none found
     */
    std::shared_ptr<MockHTTP2Request> getDownchannelRequest(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * Check if a pause is received while sending data.
     *
     * @return True if a pause is received, false otherwise
     */
    bool isPauseOnSendReceived(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /*
     * Get the number of POST requests in the queue.
     *
     * @return The number of POST requests in the queue.
     */
    std::size_t getPostRequestsNum();

    /*
     * Get the number of HTTP2 requests in the queue.
     *
     * @return The number of HTTP2 requests in the queue.
     */
    std::size_t getRequestsNum();

    /*
     * Get the number of downchannel requests in the queue.
     *
     * @return The number of downchannel requests in the queue.
     */
    std::size_t getDownchannelRequestsNum();

    /*
     * Pop the oldest HTTP2 POST request from queue if it is not empty, otherwise wait for at most @c timeout for a
     * request to be enqueued
     *
     * @return The oldest HTTP2 POST request in the queue or nullptr if the timeout expires
     */
    std::shared_ptr<MockHTTP2Request> dequePostRequest(const std::chrono::milliseconds timeout);

    /*
     * Pop the oldest HTTP2 POST request from queue.
     *
     * @return The oldest HTTP2 POST request in the queue.
     */
    std::shared_ptr<MockHTTP2Request> dequePostRequest();

    /*
     * Pop the oldest HTTP2 Ping request from queue.
     *
     * @return The oldest HTTP2 Ping request in the queue.
     */
    std::shared_ptr<MockHTTP2Request> dequePingRequest();

    /*
     * Retrieve the maximum number of POST requests in the queue at any given time.
     *
     * @return The maximum number of POST requests in the queue at any given time.
     */
    std::size_t getMaxPostRequestsEnqueud();

private:
    /// Queue of HTTP2 requests from @c HTTP2Transport. Serialized by @c m_requestMutex.
    std::deque<std::shared_ptr<MockHTTP2Request>> m_requestQueue;

    /// Serializes concurrent access to the m_requestQueue and m_isStopping members.
    std::mutex m_requestMutex;

    /// Used to wake a thread that processes HTTP2Request in @c m_requestQueue.
    std::condition_variable m_requestCv;

    /// A string that identifies the downchannel URL.
    std::string m_downchannelURL;

    /// A string that identifies the ping URL.
    std::string m_pingURL;

    /// Queue of HTTP2 requests that are only for the downchannel. Serialized by @c m_downchannelRequestMutex.
    std::deque<std::shared_ptr<MockHTTP2Request>> m_downchannelRequestQueue;

    /// Serializes access to receiving downchannel requests.
    std::mutex m_downchannelRequestMutex;

    // Used to wake a thread that is waiting for a downchannel request.
    std::condition_variable m_downchannelRequestCv;

    /// Queue of HTTP2 POST requests. Serialized by @c m_postRequestMutex.
    std::deque<std::shared_ptr<MockHTTP2Request>> m_postRequestQueue;

    /// Serializes access to receiving POST requests.
    std::mutex m_postRequestMutex;

    /// Used to wake a thread that is waiting a POST HTTP2Request.
    std::condition_variable m_requestPostCv;

    /// Queue of Ping requests. Serialized by @c m_pingRequestMutex.
    std::deque<std::shared_ptr<MockHTTP2Request>> m_pingRequestQueue;

    /// Serializes access to receiving Ping requests.
    std::mutex m_pingRequestMutex;

    // Used to wake a thread that is waiting for a Ping request.
    std::condition_variable m_pingRequestCv;

    /// A string that contains the header to match in @c waitForRequestWithHeader().
    std::string m_headerMatch;

    /// Used to wake a thread that processes HTTP2Request with a header content matching @c m_headerMatch.
    std::condition_variable m_requestHeaderCv;

    /// Indicator that a pause is received while doing @c HTTP2RequestSourceInterface::onSendData()
    PromiseFuturePair<void> m_receivedPauseOnSend;

    /// The response code to be replied for every POST request received.
    http::HTTPResponseCode m_postResponseCode;

    /// The maximum number of POST requests in the queue at any given time.
    std::size_t m_maxPostRequestsEnqueued;

    /// A constant to specify the buffer size to be used in reading HTTP2 data.
    /// Set to 100 bytes for test purposes
    static const unsigned READ_DATA_BUF_SIZE = 100;
};

}  // namespace test
}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKHTTP2CONNECTION_H_

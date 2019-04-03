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

#include "MockHTTP2Connection.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {
namespace test {

using namespace avsCommon::utils::http;
    
MockHTTP2Connection::MockHTTP2Connection(std::string dURL, std::string pingURL) :
        m_downchannelURL{dURL},
        m_pingURL{pingURL},
        m_postResponseCode{HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED},
        m_maxPostRequestsEnqueued{0} {
}

std::shared_ptr<HTTP2RequestInterface> MockHTTP2Connection::createAndSendRequest(const HTTP2RequestConfig& config) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    // Create HTTP2 request from config.
    auto request = std::make_shared<MockHTTP2Request>(config);

    // Add request to queue.
    m_requestQueue.push_back(request);

    // Notify any listeners that are waiting for a header match.
    if (m_headerMatch.length() > 0) {
        for (auto header : request->getSource()->getRequestHeaderLines()) {
            if (header.find(m_headerMatch) != std::string::npos) {
                m_requestHeaderCv.notify_one();
            }
        }
    }

    if (request->getRequestType() == HTTP2RequestType::POST) {
        // Parse POST HTTP2 Requests.
        std::lock_guard<std::mutex> lock(m_postRequestMutex);
        m_postRequestQueue.push_back(request);
        if (m_postResponseCode != HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED) {
            request->getSink()->onReceiveResponseCode(responseCodeToInt(m_postResponseCode));
        }
        if (m_postRequestQueue.size() > m_maxPostRequestsEnqueued) {
            m_maxPostRequestsEnqueued = m_postRequestQueue.size();
        }
        m_requestPostCv.notify_one();
    } else if (m_downchannelURL == request->getUrl()) {
        // Push downchannel requests to its queue.
        std::lock_guard<std::mutex> lock(m_downchannelRequestMutex);
        m_downchannelRequestQueue.push_back(request);
        m_downchannelRequestCv.notify_all();
    } else if (m_pingURL == request->getUrl()) {
        // Push ping requests to its queue.
        std::lock_guard<std::mutex> lock(m_pingRequestMutex);
        m_pingRequestQueue.push_back(request);
        m_pingRequestCv.notify_one();
    }

    m_requestCv.notify_one();
    return request;
}

bool MockHTTP2Connection::isRequestQueueEmpty() {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    return m_requestQueue.empty();
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::waitForRequest(
    std::chrono::milliseconds timeout,
    unsigned requestNum) {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    if (!m_requestCv.wait_for(lock, timeout, [this, requestNum] {
            return !m_requestQueue.empty() && m_requestQueue.size() >= requestNum;
        })) {
        return nullptr;
    }
    return m_requestQueue.front();
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::dequeRequest() {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    auto req = m_requestQueue.front();
    m_requestQueue.pop_front();
    return req;
}

void MockHTTP2Connection::setWaitRequestHeader(const std::string& matchString) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    m_headerMatch = matchString;
}

bool MockHTTP2Connection::waitForRequestWithHeader(std::chrono::milliseconds timeout) {
    if (waitForRequest(timeout)) {
        std::unique_lock<std::mutex> lock(m_requestMutex);
        return m_requestHeaderCv.wait_for(lock, timeout, [this] { return !m_requestQueue.empty(); });
    }

    return false;
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::waitForPostRequest(const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_postRequestMutex);

    if (!m_requestPostCv.wait_for(lock, timeout, [this] { return !m_postRequestQueue.empty(); })) {
        return nullptr;
    }

    auto request = m_postRequestQueue.back();

    // Need to send 200 to MIME decoder in order for it parse the message.
    request->getMimeDecoder()->onReceiveResponseCode(
        static_cast<std::underlying_type<HTTPResponseCode>::type>(HTTPResponseCode::SUCCESS_OK));

    // Feed the header lines to the MIME decoder.
    for (auto headerLine : request->getSource()->getRequestHeaderLines()) {
        request->getMimeDecoder()->onReceiveHeaderLine(headerLine);
    }

    // Feed the data to the MIME decoder.
    char buf[READ_DATA_BUF_SIZE] = {'\0'};
    bool stop = false;
    do {
        auto res = request->getSource()->onSendData(buf, READ_DATA_BUF_SIZE);
        switch (res.status) {
            case HTTP2SendStatus::COMPLETE:
            case HTTP2SendStatus::ABORT:
                stop = true;
                break;
            case HTTP2SendStatus::PAUSE:
                if (!isPauseOnSendReceived()) {
                    m_receivedPauseOnSend.setValue();
                }
            // fall-through
            case HTTP2SendStatus::CONTINUE:
                break;
        }
        if (stop) {
            break;
        }

        request->getMimeDecoder()->onReceiveData(buf, res.size);

    } while (true);

    return request;
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::waitForPingRequest(const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_pingRequestMutex);
    if (!m_pingRequestCv.wait_for(lock, timeout, [this] { return !m_pingRequestQueue.empty(); })) {
        return nullptr;
    }

    return m_pingRequestQueue.back();
}

bool MockHTTP2Connection::respondToDownchannelRequests(
    long responseCode,
    bool sendResponseFinished,
    const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_downchannelRequestMutex);
    auto ret = m_downchannelRequestCv.wait_for(lock, timeout, [this] { return !m_downchannelRequestQueue.empty(); });

    for (auto request : m_downchannelRequestQueue) {
        request->getSink()->onReceiveResponseCode(responseCode);
        if (sendResponseFinished) {
            request->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);
        }
    }
    return ret;
}

void MockHTTP2Connection::setResponseToPOSTRequests(HTTPResponseCode responseCode) {
    std::lock_guard<std::mutex> lock(m_postRequestMutex);
    m_postResponseCode = responseCode;
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::getDownchannelRequest(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_downchannelRequestMutex);
    m_downchannelRequestCv.wait_for(lock, timeout);
    if (m_downchannelRequestQueue.empty()) {
        return nullptr;
    }
    return m_downchannelRequestQueue.back();
}

bool MockHTTP2Connection::isPauseOnSendReceived(std::chrono::milliseconds timeout) {
    return m_receivedPauseOnSend.waitFor(timeout);
}

std::size_t MockHTTP2Connection::getPostRequestsNum() {
    return m_postRequestQueue.size();
}

std::size_t MockHTTP2Connection::getRequestsNum() {
    return m_requestQueue.size();
}

std::size_t MockHTTP2Connection::getDownchannelRequestsNum() {
    return m_downchannelRequestQueue.size();
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::dequePostRequest() {
    std::lock_guard<std::mutex> lock(m_postRequestMutex);
    if (m_postRequestQueue.empty()) {
        return nullptr;
    }
    auto req = m_postRequestQueue.front();
    m_postRequestQueue.pop_front();
    return req;
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::dequePostRequest(const std::chrono::milliseconds timeout) {
    auto req = dequePostRequest();

    if (!req) {
        if (waitForPostRequest(timeout) == nullptr) {
            return nullptr;
        }
        req = dequePostRequest();
    }

    return req;
}

std::shared_ptr<MockHTTP2Request> MockHTTP2Connection::dequePingRequest() {
    std::lock_guard<std::mutex> lock(m_pingRequestMutex);
    if (m_pingRequestQueue.empty()) {
        return nullptr;
    }
    auto req = m_pingRequestQueue.front();
    m_pingRequestQueue.pop_front();
    return req;
}

std::size_t MockHTTP2Connection::getMaxPostRequestsEnqueud() {
    return m_maxPostRequestsEnqueued;
}

}  // namespace test
}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

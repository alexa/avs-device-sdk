/*
 * HTTP2Stream.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <AVSCommon/Utils/Logger/Logger.h>
#include "ACL/Transport/HTTP2Stream.h"
#include "ACL/Transport/HTTP2Transport.h"

#include <sstream>
#include <cstdint>

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2Stream");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// MIME boundary string prefix in HTTP header.
static const std::string BOUNDARY_PREFIX = "boundary=";
/// Size in chars of the MIME boundary string prefix
static const int BOUNDARY_PREFIX_SIZE = BOUNDARY_PREFIX.size();
/// MIME HTTP header value delimiter
static const std::string BOUNDARY_DELIMITER = ";";
/// The HTTP header to pass the LWA token into
static const std::string AUTHORIZATION_HEADER = "Authorization: Bearer ";
/// The POST field name for an attachment
static const std::string ATTACHMENT_FIELD_NAME = "audio";
/// The POST field name for message metadata
static const std::string METADATA_FIELD_NAME = "metadata";
/// The prefix for a stream contextId.
static const std::string STREAM_CONTEXT_ID_PREFIX_STRING = "ACL_LOGICAL_HTTP2_STREAM_ID_";
/// The prefix of request IDs passed back in the header of AVS replies.
static const std::string X_AMZN_REQUESTID_PREFIX = "x-amzn-requestid:";
#ifdef DEBUG
/// Carriage return
static const char CR = 0x0D;
#endif

// Definition for the class static member variable.
unsigned int HTTP2Stream::m_streamIdCounter = 1;

/**
 * A local function to help us emulate HTTP/2 stream ids increasing by two when incrementing.
 * Invoking this function within an initialization section is more readable than writing something like {++(++id)}.
 */
static unsigned int incrementCounterByTwo(unsigned int* id) {
    *id += 2;
    return *id;
}

/**
 * Get @c std::chrono::steady_clock::now() in a form that can be wrapped in @c atomic.
 *
 * @return @c std::chrono::steady_clock::now() in a form that can be wrapped in @c atomic.
 */
static std::chrono::steady_clock::rep getNow() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

HTTP2Stream::HTTP2Stream(MessageConsumerInterface* messageConsumer,
         std::shared_ptr<AttachmentManager> attachmentManager)
    : m_logicalStreamId{incrementCounterByTwo(&m_streamIdCounter)},
      m_parser{messageConsumer, attachmentManager, STREAM_CONTEXT_ID_PREFIX_STRING + std::to_string(m_logicalStreamId)},
      m_isPaused{false},
      m_progressTimeout{std::chrono::steady_clock::duration::max().count()},
      m_timeOfLastTransfer{getNow()} {
}

bool HTTP2Stream::reset() {
    if (!m_transfer.reset()) {
        ACSDK_ERROR(LX("resetFailed").d("reason", "resetHandleFailed"));
        return false;
    }
    m_currentRequest = nullptr;
    m_parser.reset();
    m_progressTimeout = std::chrono::steady_clock::duration::max().count();
    m_timeOfLastTransfer = getNow();
    return true;
}

bool HTTP2Stream::setCommonOptions(const std::string& url, const std::string& authToken) {
    CURLcode ret;
    std::ostringstream authHeader;
    authHeader << AUTHORIZATION_HEADER << authToken;

    if (!m_transfer.setURL(url)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed")
                .d("reason", "setURLFailed")
                .d("url", url));
        return false;
    }

    if (!m_transfer.addHTTPHeader(authHeader.str())) {
        ACSDK_ERROR(LX("setCommonOptionsFailed")
                .d("reason", "addHTTPHeaderFailed")
                .sensitive("authHeader", authHeader.str()));
        return false;
    }

    if (!m_transfer.setWriteCallback(&HTTP2Stream::writeCallback, this)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed").d("reason", "setWriteCallbackFailed"));
        return false;
    }

    if (!m_transfer.setHeaderCallback(&HTTP2Stream::headerCallback, this)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed").d("reason", "setHeaderCallbackFailed"));
        return false;
    }
#ifdef ACSDK_EMIT_SENSITIVE_LOGS
    ret = curl_easy_setopt(m_transfer.getCurlHandle(), CURLOPT_VERBOSE, 1L);
    if (ret != CURLE_OK) {
        ACSDK_ERROR(LX("setCommonOptionsFailed")
                .d("reason", "curlFailure")
                .d("method", "curl_easy_setopt")
                .d("option", "CURLOPT_VERBOSE")
                .d("error", curl_easy_strerror(ret)));
        return false;
    }
#endif
    ret = curl_easy_setopt(m_transfer.getCurlHandle(), CURLOPT_TCP_KEEPALIVE, 1);
    // Set TCP_KEEPALIVE to ensure that we detect server initiated disconnects
    if (ret != CURLE_OK) {
        ACSDK_ERROR(LX("setCommonOptionsFailed")
                .d("reason", "curlFailure")
                .d("method", "curl_easy_setopt")
                .d("option", "CURLOPT_TCP_KEEPALIVE")
                .d("error", curl_easy_strerror(ret)));
        return false;
    }
    return true;
}

bool HTTP2Stream::initGet(const std::string& url ,const std::string& authToken) {
    reset();

    if (url.empty()) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "emptyURL"));
        return false;
    }

    if (authToken.empty()) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "emptyAuthToken"));
        return false;
    }

    if (!m_transfer.getCurlHandle()) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "curlEasyHandleUninitialized"));
        return false;
    }

    if (!m_transfer.setTransferType(CurlEasyHandleWrapper::TransferType::kGET)) {
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "setCommonOptionsFailed"));
        return false;
    }

    return true;
}

bool HTTP2Stream::initPost(const std::string& url, const std::string& authToken,
                        std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    reset();
    std::string requestPayload = request->getJsonContent();

    if (url.empty()) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "emptyURL"));
        return false;
    }

    if (authToken.empty()) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "emptyAuthToken"));
        return false;
    }

    if (!request) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "nullMessageRequest"));
        return false;
    }

    if (!m_transfer.getCurlHandle()) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "curlEasyHandleUninitialized"));
        return false;
    }

    if (!m_transfer.setPostContent(METADATA_FIELD_NAME, requestPayload)) {
        return false;
    }

    if (!m_transfer.setReadCallback(HTTP2Stream::readCallback, this)) {
        return false;
    }

    if (request->getAttachmentReader()) {
        if (!m_transfer.setPostStream(ATTACHMENT_FIELD_NAME, this)) {
            return false;
        }
    }

    if (!m_transfer.setTransferType(CurlEasyHandleWrapper::TransferType::kPOST)) {
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setCommonOptionsFailed"));
        return false;
    }

    m_currentRequest = request;
    return true;
}

size_t HTTP2Stream::writeCallback(char *data, size_t size, size_t nmemb, void *user) {
    size_t numChars = size * nmemb;
    HTTP2Stream *stream = static_cast<HTTP2Stream*>(user);
    stream->m_timeOfLastTransfer = getNow();
    /**
     * If we get an HTTP 200 response code then we're getting a MIME multipart
     * payload. For all other response codes we're getting a JSON string without
     * multipart headers.
     */
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_OK == stream->getResponseCode()) {
        MimeParser::DataParsedStatus status = stream->m_parser.feed(data, numChars);

        if (MimeParser::DataParsedStatus::OK == status) {
            return numChars;
        } else if (MimeParser::DataParsedStatus::INCOMPLETE == status) {
            stream->m_isPaused = true;
            return CURL_READFUNC_PAUSE;
        } else if (MimeParser::DataParsedStatus::ERROR == status) {
            return CURL_READFUNC_ABORT;
        }
    } else {
        stream->m_exceptionBeingProcessed.append(data, numChars);
    }
    return numChars;
}

size_t HTTP2Stream::headerCallback(char *data, size_t size, size_t nmemb, void *user) {
    size_t headerLength = size * nmemb;
    std::string header(data, headerLength);
#ifdef DEBUG
    if (0 == header.find(X_AMZN_REQUESTID_PREFIX)) {
        auto end = header.find(CR);
        ACSDK_DEBUG(LX("receivedRequestId").d("value", header.substr(0, end)));
    }
#endif
    std::string boundary;
    HTTP2Stream *stream = static_cast<HTTP2Stream*>(user);
    stream->m_timeOfLastTransfer = getNow();
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_OK == stream->getResponseCode()) {
        if (header.find(BOUNDARY_PREFIX) != std::string::npos) {
            boundary = header.substr(header.find(BOUNDARY_PREFIX));
            boundary = boundary.substr(BOUNDARY_PREFIX_SIZE,
                            boundary.find(BOUNDARY_DELIMITER) - BOUNDARY_PREFIX_SIZE);
            stream->m_parser.setBoundaryString(boundary);
        }
    }
    return headerLength;
}

size_t HTTP2Stream::readCallback(char *data, size_t size, size_t nmemb, void *userData) {
    HTTP2Stream *stream = static_cast<HTTP2Stream*>(userData);
    stream->m_timeOfLastTransfer = getNow();

    auto attachmentReader = stream->m_currentRequest->getAttachmentReader();

    // This is ok - it means there's no attachment to send.  Return 0 so libcurl can complete the stream to AVS.
    if (!attachmentReader) {
        return 0;
    }

    // Pass the data to libcurl.
    const size_t maxBytesToRead = size * nmemb;
    auto readStatus = AttachmentReader::ReadStatus::OK;
    auto bytesRead = attachmentReader->read(data, maxBytesToRead, &readStatus);

    switch (readStatus) {

        // The good cases.
        case AttachmentReader::ReadStatus::OK:
        case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
        case AttachmentReader::ReadStatus::OK_TIMEDOUT:
            break;

        // No more data to send - close the stream.
        case AttachmentReader::ReadStatus::CLOSED:
            return 0;

        // Handle any attachment read errors.
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
            return CURL_READFUNC_ABORT;
    }

    // The attachment has no more data right now, but is still readable.
    if (0 == bytesRead) {
        stream->setPaused(true);
        return CURL_READFUNC_PAUSE;
    }

    return bytesRead;
}

long HTTP2Stream::getResponseCode() {
    long responseCode = 0;
    CURLcode ret = curl_easy_getinfo(m_transfer.getCurlHandle(), CURLINFO_RESPONSE_CODE, &responseCode);
    if (ret != CURLE_OK) {
        ACSDK_ERROR(LX("getResponseCodeFailed")
                .d("reason", "curlFailure")
                .d("method", "curl_easy_getinfo")
                .d("info", "CURLINFO_RESPONSE_CODE")
                .d("error", curl_easy_strerror(ret)));
        return -1;
    }
    return responseCode;
}

CURL* HTTP2Stream::getCurlHandle() {
    return m_transfer.getCurlHandle();
}

void HTTP2Stream::notifyRequestObserver() {
    if (m_exceptionBeingProcessed.length() > 0) {
        m_currentRequest->onExceptionReceived(m_exceptionBeingProcessed);
        m_exceptionBeingProcessed = "";
    }

    long responseCode = getResponseCode();

    switch (responseCode) {
        case HTTP2Stream::HTTPResponseCodes::NO_RESPONSE_RECEIVED:
            m_currentRequest->onSendCompleted(avsCommon::avs::MessageRequest::Status::INTERNAL_ERROR);
            break;
        case HTTP2Stream::HTTPResponseCodes::SUCCESS_OK:
        case HTTP2Stream::HTTPResponseCodes::SUCCESS_NO_CONTENT:
            m_currentRequest->onSendCompleted(avsCommon::avs::MessageRequest::Status::SUCCESS);
            break;
        default:
            m_currentRequest->onSendCompleted(avsCommon::avs::MessageRequest::Status::SERVER_INTERNAL_ERROR);
    }
}

void HTTP2Stream::notifyRequestObserver(avsCommon::avs::MessageRequest::Status status) {
    m_currentRequest->onSendCompleted(status);
}

bool HTTP2Stream::setStreamTimeout(const long timeoutSeconds) {
    if (!m_transfer.setTransferTimeout(timeoutSeconds)) {
        return false;
    }
    return true;
}

bool HTTP2Stream::setConnectionTimeout(const std::chrono::seconds timeoutSeconds) {
    return m_transfer.setConnectionTimeout(timeoutSeconds);
}

void HTTP2Stream::setPaused(bool isPaused) {
    if (m_isPaused && !isPaused) {
        curl_easy_pause(getCurlHandle(), CURLPAUSE_CONT);
    }
    m_isPaused = isPaused;
}

bool HTTP2Stream::isPaused() const {
    return m_isPaused;
}

unsigned int HTTP2Stream::getLogicalStreamId() const {
    return m_logicalStreamId;
}

bool HTTP2Stream::hasProgressTimedOut() const {
    return (getNow() - m_timeOfLastTransfer) > m_progressTimeout;
}

} // namespace acl
} // namespace alexaClientSDK

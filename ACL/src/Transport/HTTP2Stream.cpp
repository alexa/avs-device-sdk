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

/**
 * Get @c std::chrono::steady_clock::now() in a form that can be wrapped in @c atomic.
 *
 * @return @c std::chrono::steady_clock::now() in a form that can be wrapped in @c atomic.
 */
static std::chrono::steady_clock::rep getNow() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

HTTP2Stream::HTTP2Stream(
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<AttachmentManager> attachmentManager) :
        m_logicalStreamId{0},
        m_parser{messageConsumer, attachmentManager},
        m_hasSendCompleted{false},
        m_isNetworkSendBlockedOnLocalRead{false},
        m_hasReceiveStarted{false},
        m_isNetworkReceiveBlockedOnLocalWrite{false},
        m_progressTimeout{std::chrono::steady_clock::duration::max().count()},
        m_timeOfLastTransfer{getNow()} {
}

bool HTTP2Stream::reset() {
    if (!m_transfer.reset()) {
        ACSDK_ERROR(LX("resetFailed").d("reason", "resetHandleFailed"));
        return false;
    }
    m_parser.reset();
    m_currentRequest.reset();
    m_hasSendCompleted = false;
    m_isNetworkSendBlockedOnLocalRead = false;
    m_hasReceiveStarted = false;
    m_isNetworkReceiveBlockedOnLocalWrite = false;
    m_exceptionBeingProcessed.clear();
    m_progressTimeout = std::chrono::steady_clock::duration::max().count();
    m_timeOfLastTransfer = getNow();
    return true;
}

bool HTTP2Stream::setCommonOptions(const std::string& url, const std::string& authToken) {
    CURLcode ret;
    std::ostringstream authHeader;
    authHeader << AUTHORIZATION_HEADER << authToken;

    if (!m_transfer.setURL(url)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed").d("reason", "setURLFailed").d("url", url));
        return false;
    }

    if (!m_transfer.addHTTPHeader(authHeader.str())) {
        ACSDK_ERROR(
            LX("setCommonOptionsFailed").d("reason", "addHTTPHeaderFailed").sensitive("authHeader", authHeader.str()));
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

bool HTTP2Stream::initGet(const std::string& url, const std::string& authToken) {
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

bool HTTP2Stream::initPost(
    const std::string& url,
    const std::string& authToken,
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
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setPostContentFailed"));
        return false;
    }

    if (!m_transfer.setReadCallback(HTTP2Stream::readCallback, this)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setReadCallbackFailed"));
        return false;
    }

    if (request->getAttachmentReader()) {
        if (!m_transfer.setPostStream(ATTACHMENT_FIELD_NAME, this)) {
            ACSDK_ERROR(LX("initPostFailed").d("reason", "setPostStreamFailed"));
            return false;
        }
    }

    if (!m_transfer.setTransferType(CurlEasyHandleWrapper::TransferType::kPOST)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setTransferTypeFailed"));
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setCommonOptionsFailed"));
        return false;
    }

    m_currentRequest = request;
    return true;
}

size_t HTTP2Stream::writeCallback(char* data, size_t size, size_t nmemb, void* user) {
    size_t numChars = size * nmemb;
    HTTP2Stream* stream = static_cast<HTTP2Stream*>(user);
    stream->m_timeOfLastTransfer = getNow();
    stream->m_hasReceiveStarted = true;

    /**
     * If we get an HTTP 200 response code then we're getting a MIME multipart
     * payload. For all other response codes we're getting a JSON string without
     * multipart headers.
     */
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_OK == stream->getResponseCode()) {
        MimeParser::DataParsedStatus status = stream->m_parser.feed(data, numChars);

        if (MimeParser::DataParsedStatus::OK == status) {
            if (stream->m_isNetworkReceiveBlockedOnLocalWrite) {
                stream->m_isNetworkReceiveBlockedOnLocalWrite = false;
                ACSDK_DEBUG9(LX("writeCallback").d("blocked", false).d("streamId", stream->m_logicalStreamId));
            }
            return numChars;
        } else if (MimeParser::DataParsedStatus::INCOMPLETE == status) {
            if (!stream->m_isNetworkReceiveBlockedOnLocalWrite) {
                stream->m_isNetworkReceiveBlockedOnLocalWrite = true;
                ACSDK_DEBUG9(LX("writeCallback").d("blocked", true).d("streamId", stream->m_logicalStreamId));
            }
            return CURL_WRITEFUNC_PAUSE;
        } else if (MimeParser::DataParsedStatus::ERROR == status) {
            stream->m_isNetworkReceiveBlockedOnLocalWrite = false;
            return CURL_READFUNC_ABORT;
        }
    } else {
        stream->m_isNetworkReceiveBlockedOnLocalWrite = false;
        stream->m_exceptionBeingProcessed.append(data, numChars);
    }
    return numChars;
}

size_t HTTP2Stream::headerCallback(char* data, size_t size, size_t nmemb, void* user) {
    if (!user) {
        ACSDK_ERROR(LX("headerCallbackFailed").d("reason", "nullUser"));
        return 0;
    }
    size_t headerLength = size * nmemb;
    std::string header(data, headerLength);
#ifdef DEBUG
    if (0 == header.find(X_AMZN_REQUESTID_PREFIX)) {
        auto end = header.find(CR);
        ACSDK_DEBUG(LX("receivedRequestId").d("value", header.substr(0, end)));
    }
#endif
    std::string boundary;
    HTTP2Stream* stream = static_cast<HTTP2Stream*>(user);
    stream->m_timeOfLastTransfer = getNow();
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_OK == stream->getResponseCode()) {
        if (header.find(BOUNDARY_PREFIX) != std::string::npos) {
            boundary = header.substr(header.find(BOUNDARY_PREFIX));
            boundary = boundary.substr(BOUNDARY_PREFIX_SIZE, boundary.find(BOUNDARY_DELIMITER) - BOUNDARY_PREFIX_SIZE);
            stream->m_parser.setBoundaryString(boundary);
        }
    }
    return headerLength;
}

size_t HTTP2Stream::readCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX("readCallbackFailed").d("reason", "nullUserData"));
        return 0;
    }

    HTTP2Stream* stream = static_cast<HTTP2Stream*>(userData);

    stream->m_timeOfLastTransfer = getNow();
    auto attachmentReader = stream->m_currentRequest->getAttachmentReader();

    // This is ok - it means there's no attachment to send.  Return 0 so libcurl can complete the stream to AVS.
    if (!attachmentReader) {
        stream->m_hasSendCompleted = true;
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
            stream->m_hasSendCompleted = true;
            return 0;

        // Handle any attachment read errors.
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
            stream->m_isNetworkSendBlockedOnLocalRead = false;
            return CURL_READFUNC_ABORT;
    }
    // The attachment has no more data right now, but is still readable.
    if (0 == bytesRead) {
        if (!stream->m_isNetworkSendBlockedOnLocalRead) {
            stream->m_isNetworkSendBlockedOnLocalRead = true;
            // Too noisy unless you arte debugging blocked reads.
            // ACSDK_DEBUG9(LX("readCallback").d("blocked", true).d("streamId", stream->m_logicalStreamId));
        }
        return CURL_READFUNC_PAUSE;
    } else if (stream->m_isNetworkSendBlockedOnLocalRead) {
        stream->m_isNetworkSendBlockedOnLocalRead = false;
        // Too noisy unless you are debugging blocked reads.
        // ACSDK_DEBUG9(LX("readCallback").d("blocked", false).d("streamId", stream->m_logicalStreamId));
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
        m_currentRequest->exceptionReceived(m_exceptionBeingProcessed);
        m_exceptionBeingProcessed = "";
    }

    long responseCode = getResponseCode();

    switch (responseCode) {
        case HTTP2Stream::HTTPResponseCodes::NO_RESPONSE_RECEIVED:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR);
            break;
        case HTTP2Stream::HTTPResponseCodes::SUCCESS_OK:
        case HTTP2Stream::HTTPResponseCodes::SUCCESS_NO_CONTENT:
            m_currentRequest->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
            break;
        default:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR);
    }
}

void HTTP2Stream::notifyRequestObserver(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    m_currentRequest->sendCompleted(status);
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

void HTTP2Stream::resumeNetworkIO() {
    curl_easy_pause(getCurlHandle(), CURLPAUSE_CONT);
}

bool HTTP2Stream::isBlockedOnLocalIO() const {
    if (m_hasSendCompleted) {
        if (m_hasReceiveStarted) {
            return m_isNetworkReceiveBlockedOnLocalWrite;
        } else {
            return false;
        }
    } else {
        if (m_hasReceiveStarted) {
            return m_isNetworkSendBlockedOnLocalRead && m_isNetworkReceiveBlockedOnLocalWrite;
        } else {
            return m_isNetworkSendBlockedOnLocalRead;
        }
    }
}

void HTTP2Stream::setLogicalStreamId(int logicalStreamId) {
    m_logicalStreamId = logicalStreamId;
    m_parser.setAttachmentContextId(STREAM_CONTEXT_ID_PREFIX_STRING + std::to_string(m_logicalStreamId));
}

unsigned int HTTP2Stream::getLogicalStreamId() const {
    return m_logicalStreamId;
}

bool HTTP2Stream::hasProgressTimedOut() const {
    return !isBlockedOnLocalIO() && ((getNow() - m_timeOfLastTransfer) > m_progressTimeout);
}

}  // namespace acl
}  // namespace alexaClientSDK

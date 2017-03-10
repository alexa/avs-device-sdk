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
#include "AVSUtils/Logging/Logger.h"
#include "ACL/Transport/HTTP2Stream.h"
#include "ACL/Transport/HTTP2Transport.h"

#include <sstream>
#include <cstdint>

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsUtils;

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

HTTP2Stream::HTTP2Stream(MessageConsumerInterface* messageConsumer,
        std::shared_ptr<AttachmentManagerInterface> attachmentManager)
    : m_parser{messageConsumer, attachmentManager} {
}

bool HTTP2Stream::reset() {
    if (!m_transfer.reset()) {
        Logger::log("Could not reset curl easy handle");
        return false;
    }
    m_currentRequest = nullptr;
    m_parser.reset();
    return true;
}

bool HTTP2Stream::setCommonOptions(const std::string& url, const std::string& authToken) {
    std::ostringstream authHeader;
    authHeader << AUTHORIZATION_HEADER << authToken;

    if (!m_transfer.setURL(url)) {
        Logger::log("Could not set request URL");
        return false;
    }

    if (!m_transfer.addHTTPHeader(authHeader.str())) {
        Logger::log("Could not set requested HTTP header");
        return false;
    }

    if (!m_transfer.setWriteCallback(&HTTP2Stream::writeCallback, this)) {
        Logger::log("Could not set write callback");
        return false;
    }

    if (!m_transfer.setHeaderCallback(&HTTP2Stream::headerCallback, this)) {
        Logger::log("Could not set header callback");
        return false;
    }
#ifdef DEBUG
    if (curl_easy_setopt(m_transfer.getCurlHandle(), CURLOPT_VERBOSE, 1L) != CURLE_OK) {
        Logger::log("Could not set verbose logging");
        return false;
    }
#endif
    // Set TCP_KEEPALIVE to ensure that we detect server initiated disconnects
    if (curl_easy_setopt(m_transfer.getCurlHandle(), CURLOPT_TCP_KEEPALIVE, 1) != CURLE_OK) {
        Logger::log("Could not set TCP KEEPALIVE");
        return false;
    }
    return true;
}

bool HTTP2Stream::initGet(const std::string& url ,const std::string& authToken) {
    reset();

    if (!m_transfer.getCurlHandle()) {
        Logger::log("Stream curl easy handle not initialised");
        return false;
    }

    if (!m_transfer.setTransferType(CurlEasyHandleWrapper::TransferType::kGET)) {
        Logger::log("Could not set stream to GET");
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        Logger::log("Could not set common stream options");
        return false;
    }

    return true;
}

bool HTTP2Stream::initPost(const std::string& url, const std::string& authToken,
                        std::shared_ptr<MessageRequest> request) {
    reset();
    std::string requestPayload = request->getMessage()->getJSONContent();

    if (!m_transfer.getCurlHandle()) {
        Logger::log("Stream curl easy handle not initialised");
        return false;
    }

    if (!m_transfer.setPostContent(METADATA_FIELD_NAME, requestPayload)) {
        return false;
    }

    if (!m_transfer.setReadCallback(HTTP2Stream::readCallback, this)) {
        return false;
    }

    if (request->getMessage()->getAttachment()) {
        if (!m_transfer.setPostStream(ATTACHMENT_FIELD_NAME, this)) {
            return false;
        }
    }

    if (!m_transfer.setTransferType(CurlEasyHandleWrapper::TransferType::kPOST)) {
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        Logger::log("Could not set common stream options");
        return false;
    }

    m_currentRequest = request;
    return true;
}

size_t HTTP2Stream::writeCallback(char *data, size_t size, size_t nmemb, void *user) {
    size_t numChars = size * nmemb;
    HTTP2Stream *stream = static_cast<HTTP2Stream*>(user);
    /**
     * If we get an HTTP 200 response code then we're getting a MIME multipart
     * payload. For all other response codes we're getting a JSON string without
     * multipart headers.
     */
    if (HTTP2Stream::HTTPResponseCodes::SUCCESS_OK == stream->getResponseCode()) {
        stream->m_parser.feed(data, numChars);
    } else {
        auto JSONContent = std::string(data, numChars);
        auto avsException = std::make_shared<Message>(JSONContent);
        stream->m_currentRequest->onExceptionReceived(avsException);
    }
    return numChars;
}

size_t HTTP2Stream::headerCallback(char *data, size_t size, size_t nmemb, void *user) {
    size_t headerLength = size * nmemb;
    std::string header(data, headerLength);
    std::string boundary;
    HTTP2Stream *stream = static_cast<HTTP2Stream*>(user);
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
    std::shared_ptr<std::istream> dataStream = stream->m_currentRequest->getMessage()->getAttachment();

    if (dataStream->eof()) {
        return 0;
    }
    const size_t maxBytesToRead = size * nmemb;
    dataStream->read(data, maxBytesToRead);
    if (dataStream->bad()) {
        Logger::log("Failed to read attachment");
        return CURL_READFUNC_ABORT;
    }
    std::streamsize bytesRead = dataStream->gcount();
    //TODO: ACSDK-78 Handle when the stream has no bytes to read (async sources)
    return bytesRead;
}


long HTTP2Stream::getResponseCode() {
    long responseCode = 0;
    if (curl_easy_getinfo(m_transfer.getCurlHandle(), CURLINFO_RESPONSE_CODE, &responseCode) != CURLE_OK) {
        Logger::log("Could not get HTTP response code");
        return -1;
    }
    return responseCode;
}

CURL* HTTP2Stream::getCurlHandle() {
    return m_transfer.getCurlHandle();
}

void HTTP2Stream::notifyRequestObserver() {
    long responseCode = getResponseCode();

    switch (responseCode) {
        case HTTP2Stream::HTTPResponseCodes::NO_RESPONSE_RECEIVED:
            m_currentRequest->onSendCompleted(SendMessageStatus::INTERNAL_ERROR);
            break;
        case HTTP2Stream::HTTPResponseCodes::SUCCESS_OK:
        case HTTP2Stream::HTTPResponseCodes::SUCCESS_NO_CONTENT:
            m_currentRequest->onSendCompleted(SendMessageStatus::SUCCESS);
            break;
        default:
            m_currentRequest->onSendCompleted(SendMessageStatus::SERVER_INTERNAL_ERROR);
    }
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

} // namespace acl
} // namespace alexaClientSDK

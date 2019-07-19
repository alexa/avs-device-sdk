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

#include <algorithm>
#include <functional>
#include <unordered_map>

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeRequestEncoder.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/MimeResponseSink.h"
#include "ACL/Transport/MessageRequestHandler.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::http2;

/// URL to send events to
const static std::string AVS_EVENT_URL_PATH_EXTENSION = "/v20160207/events";

/// Boundary for mime encoded requests
const static std::string MIME_BOUNDARY = "WhooHooZeerOoonie!";

/// Timeout for transmission of data on a given stream
static const std::chrono::seconds STREAM_PROGRESS_TIMEOUT = std::chrono::seconds(30);

/// Mime header strings for mime parts containing json payloads.
static const std::vector<std::string> JSON_MIME_PART_HEADER_LINES = {
    "Content-Disposition: form-data; name=\"metadata\"",
    "Content-Type: application/json"};

/// Mime Content-Disposition line before name.
static const std::string CONTENT_DISPOSITION_PREFIX = "Content-Disposition: form-data; name=\"";

/// Mime Content-Disposition line after name.
static const std::string CONTENT_DISPOSITION_SUFFIX = "\"";

/// Mime Content-Type for attchments.
static const std::string ATTACHMENT_CONTENT_TYPE = "Content-Type: application/octet-stream";

/// Prefix for the ID of message requests.
static const std::string MESSAGEREQUEST_ID_PREFIX = "AVSEvent-";

/// String to identify log entries originating from this file.
static const std::string TAG("MessageRequestHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MessageRequestHandler::~MessageRequestHandler() {
    reportMessageRequestAcknowledged();
    reportMessageRequestFinished();
}

std::shared_ptr<MessageRequestHandler> MessageRequestHandler::create(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken,
    std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest,
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager) {
    ACSDK_DEBUG5(LX(__func__).d("context", context.get()).d("messageRequest", messageRequest.get()));

    if (!context) {
        ACSDK_CRITICAL(LX("MessageRequestHandlerCreateFailed").d("reason", "nullHttp2Transport"));
        return nullptr;
    }

    if (authToken.empty()) {
        ACSDK_DEBUG9(LX("createFailed").d("reason", "emptyAuthToken"));
        return nullptr;
    }

    std::shared_ptr<MessageRequestHandler> handler(new MessageRequestHandler(context, authToken, messageRequest));

    // Allow custom path extension, if provided by the sender of the MessageRequest

    auto url = context->getEndpoint();
    if (messageRequest->getUriPathExtension().empty()) {
        url += AVS_EVENT_URL_PATH_EXTENSION;
    } else {
        url += messageRequest->getUriPathExtension();
    }

    HTTP2RequestConfig cfg{HTTP2RequestType::POST, url, MESSAGEREQUEST_ID_PREFIX};
    cfg.setRequestSource(std::make_shared<HTTP2MimeRequestEncoder>(MIME_BOUNDARY, handler));
    cfg.setResponseSink(std::make_shared<HTTP2MimeResponseDecoder>(
        std::make_shared<MimeResponseSink>(handler, messageConsumer, attachmentManager, cfg.getId())));
    cfg.setActivityTimeout(STREAM_PROGRESS_TIMEOUT);

    context->onMessageRequestSent();
    auto request = context->createAndSendRequest(cfg);

    if (!request) {
        handler->reportMessageRequestAcknowledged();
        handler->reportMessageRequestFinished();
        ACSDK_ERROR(LX("MessageRequestHandlerCreateFailed").d("reason", "createAndSendRequestFailed"));
        return nullptr;
    }

    return handler;
}

MessageRequestHandler::MessageRequestHandler(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken,
    std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest) :
        ExchangeHandler{context, authToken},
        m_messageRequest{messageRequest},
        m_json{messageRequest->getJsonContent()},
        m_jsonNext{m_json.c_str()},
        m_countOfJsonBytesLeft{m_json.size()},
        m_countOfPartsSent{0},
        m_wasMessageRequestAcknowledgeReported{false},
        m_wasMessageRequestFinishedReported{false},
        m_responseCode{0} {
    ACSDK_DEBUG5(LX(__func__).d("context", context.get()).d("messageRequest", messageRequest.get()));
}

void MessageRequestHandler::reportMessageRequestAcknowledged() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_wasMessageRequestAcknowledgeReported) {
        m_wasMessageRequestAcknowledgeReported = true;
        m_context->onMessageRequestAcknowledged();
    }
}

void MessageRequestHandler::reportMessageRequestFinished() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_wasMessageRequestFinishedReported) {
        m_wasMessageRequestFinishedReported = true;
        m_context->onMessageRequestFinished();
    }
}

std::vector<std::string> MessageRequestHandler::getRequestHeaderLines() {
    ACSDK_DEBUG5(LX(__func__));

    m_context->onActivity();

    return {m_authHeader};
}

HTTP2GetMimeHeadersResult MessageRequestHandler::getMimePartHeaderLines() {
    ACSDK_DEBUG5(LX(__func__));

    m_context->onActivity();

    if (0 == m_countOfPartsSent) {
        return HTTP2GetMimeHeadersResult(JSON_MIME_PART_HEADER_LINES);
    } else if (static_cast<int>(m_countOfPartsSent) <= m_messageRequest->attachmentReadersCount()) {
        m_namedReader = m_messageRequest->getAttachmentReader(m_countOfPartsSent - 1);
        if (m_namedReader) {
            return HTTP2GetMimeHeadersResult(
                {CONTENT_DISPOSITION_PREFIX + m_namedReader->name + CONTENT_DISPOSITION_SUFFIX,
                 ATTACHMENT_CONTENT_TYPE});
        } else {
            ACSDK_ERROR(LX("getMimePartHeaderLinesFailed").d("reason", "nullReader").d("index", m_countOfPartsSent));
            return HTTP2GetMimeHeadersResult::ABORT;
        }
    } else {
        return HTTP2GetMimeHeadersResult::COMPLETE;
    }
}

HTTP2SendDataResult MessageRequestHandler::onSendMimePartData(char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size));

    m_context->onActivity();

    if (0 == m_countOfPartsSent) {
        if (m_countOfJsonBytesLeft != 0) {
            size_t countToCopy = (m_countOfJsonBytesLeft <= size) ? m_countOfJsonBytesLeft : size;
            std::copy(m_jsonNext, m_jsonNext + countToCopy, bytes);
            m_jsonNext += countToCopy;
            m_countOfJsonBytesLeft -= countToCopy;
            return HTTP2SendDataResult(countToCopy);
        } else {
            m_countOfPartsSent++;
            return HTTP2SendDataResult::COMPLETE;
        }
    } else if (m_namedReader) {
        auto readStatus = AttachmentReader::ReadStatus::OK;
        auto bytesRead = m_namedReader->reader->read(bytes, size, &readStatus);
        ACSDK_DEBUG5(LX("attachmentRead").d("readStatus", (int)readStatus).d("bytesRead", bytesRead));
        switch (readStatus) {
            // The good cases.
            case AttachmentReader::ReadStatus::OK:
            case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case AttachmentReader::ReadStatus::OK_TIMEDOUT:
                return bytesRead != 0 ? HTTP2SendDataResult(bytesRead) : HTTP2SendDataResult::PAUSE;

            case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
                return HTTP2SendDataResult::ABORT;

            case AttachmentReader::ReadStatus::CLOSED:
                // Stream consumed.  Move on to next part.
                m_namedReader.reset();
                m_countOfPartsSent++;
                return HTTP2SendDataResult::COMPLETE;

            // Handle any attachment read errors.
            case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case AttachmentReader::ReadStatus::ERROR_INTERNAL:
                // Stream failure.  Abort sending the request.
                return HTTP2SendDataResult::ABORT;

            case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
                return HTTP2SendDataResult::PAUSE;
        }
    }

    ACSDK_ERROR(LX("onSendMimePartDataFailed").d("reason", "noMoreAttachments"));
    return HTTP2SendDataResult::ABORT;
}

void MessageRequestHandler::onActivity() {
    m_context->onActivity();
}

bool MessageRequestHandler::onReceiveResponseCode(long responseCode) {
    ACSDK_DEBUG5(LX(__func__).d("responseCode", responseCode));

    // TODO ACSDK-1839: Provide MessageRequestObserverInterface immediate notification of receipt of response code.

    reportMessageRequestAcknowledged();

    if (HTTPResponseCode::CLIENT_ERROR_FORBIDDEN == intToHTTPResponseCode(responseCode)) {
        m_context->onForbidden(m_authToken);
    }

    m_responseCode = responseCode;
    return true;
}

void MessageRequestHandler::onResponseFinished(HTTP2ResponseFinishedStatus status, const std::string& nonMimeBody) {
    ACSDK_DEBUG5(LX(__func__).d("status", status).d("responseCode", m_responseCode));

    if (HTTP2ResponseFinishedStatus::TIMEOUT == status) {
        m_context->onMessageRequestTimeout();
    }

    reportMessageRequestAcknowledged();
    reportMessageRequestFinished();

    if ((intToHTTPResponseCode(m_responseCode) != HTTPResponseCode::SUCCESS_OK) && !nonMimeBody.empty()) {
        m_messageRequest->exceptionReceived(nonMimeBody);
    }

    // Hash to allow use of HTTP2ResponseFinishedStatus as the key in an unordered_map.
    struct statusHash {
        size_t operator()(const HTTP2ResponseFinishedStatus& key) const {
            return static_cast<size_t>(key);
        }
    };

    // Mapping HTTP2ResponseFinishedStatus to a MessageRequestObserverInterface::Status.  Note that no mapping is
    // provided from the COMPLETE status so that the logic below falls through to map the HTTPResponseCode value
    // from the completed requests to the appropriate MessageRequestObserverInterface value.
    static const std::unordered_map<HTTP2ResponseFinishedStatus, MessageRequestObserverInterface::Status, statusHash>
        statusToResult = {
            {HTTP2ResponseFinishedStatus::INTERNAL_ERROR, MessageRequestObserverInterface::Status::INTERNAL_ERROR},
            {HTTP2ResponseFinishedStatus::CANCELLED, MessageRequestObserverInterface::Status::CANCELED},
            {HTTP2ResponseFinishedStatus::TIMEOUT, MessageRequestObserverInterface::Status::TIMEDOUT}};

    // Map HTTPResponseCode values to MessageRequestObserverInterface::Status values.
    static const std::unordered_map<long, MessageRequestObserverInterface::Status> responseToResult = {
        {HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED, MessageRequestObserverInterface::Status::INTERNAL_ERROR},
        {HTTPResponseCode::SUCCESS_OK, MessageRequestObserverInterface::Status::SUCCESS},
        {HTTPResponseCode::SUCCESS_NO_CONTENT, MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT},
        {HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST, MessageRequestObserverInterface::Status::BAD_REQUEST},
        {HTTPResponseCode::CLIENT_ERROR_FORBIDDEN, MessageRequestObserverInterface::Status::INVALID_AUTH},
        {HTTPResponseCode::SERVER_ERROR_INTERNAL, MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2}};

    auto result = MessageRequestObserverInterface::Status::INTERNAL_ERROR;

    if (HTTP2ResponseFinishedStatus::COMPLETE == status) {
        auto responseIterator = responseToResult.find(m_responseCode);
        if (responseIterator != responseToResult.end()) {
            result = responseIterator->second;
        } else {
            result = MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR;
        }
    } else {
        auto statusIterator = statusToResult.find(status);
        if (statusIterator != statusToResult.end()) {
            result = statusIterator->second;
        }
    }

    m_messageRequest->sendCompleted(result);
}

}  // namespace acl
}  // namespace alexaClientSDK

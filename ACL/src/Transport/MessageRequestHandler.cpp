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

#include <algorithm>
#include <functional>
#include <unordered_map>

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeRequestEncoder.h>
#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/MimeResponseSink.h"
#include "ACL/Transport/MessageRequestHandler.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::http2;
using namespace avsCommon::utils::metrics;
using namespace avsCommon::utils::power;

/// URL to send events to
const static std::string AVS_EVENT_URL_PATH_EXTENSION = "/v20160207/events";

/// Boundary for mime encoded requests
const static std::string MIME_BOUNDARY = "WhooHooZeerOoonie=";

/// Timeout for transmission of data on a given stream
static const std::chrono::seconds STREAM_PROGRESS_TIMEOUT = std::chrono::seconds(15);

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
#define TAG "MessageRequestHandler"

/// Prefix used to identify metrics published by this module.
static const std::string ACL_METRIC_SOURCE_PREFIX = "ACL-";

/// Metric identifier for send mime data error
static const std::string SEND_DATA_ERROR = "ERROR.SEND_DATA_ERROR";

/// Metric identifier for start of Mime data event being sent to the cloud.
static const std::string START_EVENT_SENT_TO_CLOUD = "START_EVENT_SENT_TO_CLOUD";

/// Read status tag
static const std::string READ_STATUS_TAG = "READ_STATUS";

/// Read overrun error
static const std::string ERROR_READ_OVERRUN = "READ_OVERRUN";

/// Internal error
static const std::string ERROR_INTERNAL = "INTERNAL_ERROR";

/// Send completed
static const std::string SEND_COMPLETED = "SEND_COMPLETED";

/// Metric identifier for message send error.
static const std::string MESSAGE_SEND_ERROR = "ERROR.MESSAGE_SEND_FAILED";

/// Key value separator for HTTP headers
static const std::string HTTP_KEY_VALUE_SEPARATOR = ": ";

/// Event header key for the namespace field.
static const std::string EVENT_HEADER_NAMESPACE = "namespace";

/// Event header key for the name field.
static const std::string EVENT_HEADER_NAME = "name";

/// Event header missing.
static const std::string EVENT_HEADER_MISSING = "EVENT_HEADER_MISSING";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Capture metric for the last send data result.
 *
 * @param metricRecorder The metric recorder object.
 * @param count Number of errors.
 * @param readStatus The read status.
 */
static void collectSendDataResultMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    int count,
    const std::string& readStatus) {
    if (!metricRecorder) {
        return;
    }

    recordMetric(
        metricRecorder,
        MetricEventBuilder{}
            .setActivityName(ACL_METRIC_SOURCE_PREFIX + SEND_DATA_ERROR)
            .addDataPoint(DataPointCounterBuilder{}.setName(SEND_DATA_ERROR).increment(count).build())
            .addDataPoint(DataPointStringBuilder{}.setName(READ_STATUS_TAG).setValue(readStatus).build())
            .build());
}

/**
 * Capture metric for cases where there are internal message send errors or timeouts.
 *
 * @param metricRecorder The metric recorder object.
 * @param status The @c MessageRequestObserverInterface::Status of the message.
 * @param messageRequest The @c MessageRequest object.
 */
static void submitMessageSendErrorMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    MessageRequestObserverInterface::Status status,
    const std::shared_ptr<MessageRequest>& messageRequest) {
    if (!metricRecorder) {
        return;
    }

    std::stringstream ss;
    switch (status) {
        case MessageRequestObserverInterface::Status::INTERNAL_ERROR:
        case MessageRequestObserverInterface::Status::TIMEDOUT:
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
        case MessageRequestObserverInterface::Status::THROTTLED:
        case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
        case MessageRequestObserverInterface::Status::REFUSED:
        case MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR:
            ss << status;
            break;
        default:
            return;
    }

    auto metricEventBuilder = MetricEventBuilder{}
                                  .setActivityName(ACL_METRIC_SOURCE_PREFIX + MESSAGE_SEND_ERROR)
                                  .addDataPoint(DataPointCounterBuilder{}.setName(ss.str()).increment(1).build());
    if (messageRequest) {
        auto eventHeaders = messageRequest->retrieveEventHeaders();
        if (!eventHeaders.eventNamespace.empty()) {
            metricEventBuilder.addDataPoint(
                DataPointStringBuilder{}.setName(EVENT_HEADER_NAMESPACE).setValue(eventHeaders.eventNamespace).build());
        } else {
            metricEventBuilder.addDataPoint(
                DataPointStringBuilder{}.setName(EVENT_HEADER_NAMESPACE).setValue(EVENT_HEADER_MISSING).build());
        }
        if (!eventHeaders.eventName.empty()) {
            metricEventBuilder.addDataPoint(
                DataPointStringBuilder{}.setName(EVENT_HEADER_NAME).setValue(eventHeaders.eventName).build());
        } else {
            metricEventBuilder.addDataPoint(
                DataPointStringBuilder{}.setName(EVENT_HEADER_NAME).setValue(EVENT_HEADER_MISSING).build());
        }
    }
    auto metricEvent = metricEventBuilder.build();
    if (!metricEvent) {
        ACSDK_ERROR(LX("submitErrorMetricFailed").d("reason", "invalid metric event"));
        return;
    }

    recordMetric(metricRecorder, metricEvent);
}

void MessageRequestHandler::recordStreamMetric(std::size_t bytes) {
    if (m_messageRequest == nullptr) {
        return;
    }

    if (m_metricRecorder == nullptr) {
        return;
    }
    if (m_recordedStreamMetric) {
        return;
    }
    m_streamBytesRead += bytes;
    std::string metricName{m_messageRequest->getStreamMetricName()};
    auto threshold = m_messageRequest->getStreamBytesThreshold();
    if (metricName == "" || threshold == 0) {
        return;
    }
    if (threshold <= m_streamBytesRead) {
        auto metricEvent = MetricEventBuilder{}
                               .setActivityName(ACL_METRIC_SOURCE_PREFIX + metricName)
                               .addDataPoint(DataPointCounterBuilder{}.setName(metricName).increment(1).build())
                               .build();
        if (!metricEvent) {
            ACSDK_ERROR(LX("recordStreamMetric").m("submitMetricFailed").d("reason", "invalid metric event"));
            return;
        }
        recordMetric(m_metricRecorder, metricEvent);
        m_recordedStreamMetric = true;
    }
}

void MessageRequestHandler::recordStartOfEventMetric() {
    if (!m_metricRecorder) {
        return;
    }
    auto metricEvent =
        MetricEventBuilder{}
            .setActivityName(ACL_METRIC_SOURCE_PREFIX + START_EVENT_SENT_TO_CLOUD)
            .addDataPoint(DataPointCounterBuilder{}.setName(START_EVENT_SENT_TO_CLOUD).increment(1).build())
            .build();
    if (!metricEvent) {
        ACSDK_ERROR(LX("recordStartOfEventMetric").m("submitMetricFailed").d("reason", "invalid metric event"));
        return;
    }
    recordMetric(m_metricRecorder, metricEvent);
}

MessageRequestHandler::~MessageRequestHandler() {
    reportMessageRequestAcknowledged();
    reportMessageRequestFinished();
    if (m_powerResource) {
        m_powerResource->release();
    }
}

std::shared_ptr<MessageRequestHandler> MessageRequestHandler::create(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken,
    std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest,
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
    std::shared_ptr<MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::EventTracerInterface> eventTracer,
    const std::shared_ptr<PowerResource>& powerResource) {
    ACSDK_DEBUG7(LX("create").d("context", context.get()).d("messageRequest", messageRequest.get()));

    if (!context) {
        ACSDK_CRITICAL(LX("MessageRequestHandlerCreateFailed").d("reason", "nullHttp2Transport"));
        return nullptr;
    }

    if (authToken.empty()) {
        ACSDK_DEBUG9(LX("createFailed").d("reason", "emptyAuthToken"));
        return nullptr;
    }

    std::shared_ptr<MessageRequestHandler> handler(
        new MessageRequestHandler(context, authToken, messageRequest, std::move(metricRecorder), powerResource));

    // Allow custom path extension, if provided by the sender of the MessageRequest

    auto url = context->getAVSGateway();
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

    context->onMessageRequestSent(messageRequest);
    auto request = context->createAndSendRequest(cfg);

    if (!request) {
        handler->reportMessageRequestAcknowledged();
        handler->reportMessageRequestFinished();
        ACSDK_ERROR(LX("MessageRequestHandlerCreateFailed").d("reason", "createAndSendRequestFailed"));
        return nullptr;
    }

    if (eventTracer) {
        eventTracer->traceEvent(messageRequest->getJsonContent());
    }
    // Log Event Message Sent
    ACSDK_DEBUG0(LX("EventSent")
                     .sensitive("url", messageRequest->getUriPathExtension())
                     .sensitive("jsonContent", messageRequest->getJsonContent()));

    return handler;
}

MessageRequestHandler::MessageRequestHandler(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken,
    std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest,
    std::shared_ptr<MetricRecorderInterface> metricRecorder,
    const std::shared_ptr<PowerResource>& powerResource) :
        ExchangeHandler{context, authToken},
        m_messageRequest{messageRequest},
        m_json{messageRequest->getJsonContent()},
        m_jsonNext{m_json.c_str()},
        m_countOfJsonBytesLeft{m_json.size()},
        m_countOfPartsSent{0},
        m_metricRecorder{metricRecorder},
        m_wasMessageRequestAcknowledgeReported{false},
        m_wasMessageRequestFinishedReported{false},
        m_responseCode{0},
        m_powerResource{powerResource},
        m_resultStatus{MessageRequestObserverInterface::Status::PENDING},
        m_streamBytesRead{0},
        m_recordedStreamMetric{false} {
    ACSDK_DEBUG7(LX("init").d("context", context.get()).d("messageRequest", messageRequest.get()));

    if (m_powerResource) {
        m_powerResource->acquire();
    }
}

void MessageRequestHandler::reportMessageRequestAcknowledged() {
    ACSDK_DEBUG7(LX("reportMessageRequestAcknowledged"));
    if (!m_wasMessageRequestAcknowledgeReported) {
        m_wasMessageRequestAcknowledgeReported = true;
        m_context->onMessageRequestAcknowledged(m_messageRequest);
    }
}

void MessageRequestHandler::reportMessageRequestFinished() {
    ACSDK_DEBUG7(LX("reportMessageRequestFinished"));
    if (!m_wasMessageRequestFinishedReported) {
        m_wasMessageRequestFinishedReported = true;
        m_context->onMessageRequestFinished();
    }
}

std::vector<std::string> MessageRequestHandler::getRequestHeaderLines() {
    ACSDK_DEBUG9(LX("getRequestHeaderLines"));

    m_context->onActivity();

    std::vector<std::string> result({m_authHeader});
    auto headers = m_messageRequest->getHeaders();
    for (const auto& header : headers) {
        result.emplace_back(header.first + HTTP_KEY_VALUE_SEPARATOR + header.second);
    }

    return result;
}

HTTP2GetMimeHeadersResult MessageRequestHandler::getMimePartHeaderLines() {
    ACSDK_DEBUG9(LX("getMimePartHeaderLines"));

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
    ACSDK_DEBUG9(LX("onSendMimePartData").d("size", size));

    m_context->onActivity();

    if (0 == m_countOfPartsSent) {
        if (m_countOfJsonBytesLeft != 0) {
            size_t countToCopy = (m_countOfJsonBytesLeft <= size) ? m_countOfJsonBytesLeft : size;
            std::copy(m_jsonNext, m_jsonNext + countToCopy, bytes);
            m_jsonNext += countToCopy;
            m_countOfJsonBytesLeft -= countToCopy;
            recordStartOfEventMetric();
            return HTTP2SendDataResult(countToCopy);
        } else {
            m_countOfPartsSent++;
            return HTTP2SendDataResult::COMPLETE;
        }
    } else if (m_namedReader) {
        auto readStatus = AttachmentReader::ReadStatus::OK;
        auto bytesRead = m_namedReader->reader->read(bytes, size, &readStatus);
        recordStreamMetric(bytesRead);
        ACSDK_DEBUG9(LX("attachmentRead").d("readStatus", (int)readStatus).d("bytesRead", bytesRead));
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
                collectSendDataResultMetric(m_metricRecorder, 0, SEND_COMPLETED);
                return HTTP2SendDataResult::COMPLETE;

            // Handle any attachment read errors.
            case AttachmentReader::ReadStatus::ERROR_OVERRUN:
                collectSendDataResultMetric(m_metricRecorder, 1, ERROR_READ_OVERRUN);
                // Stream failure.  Abort sending the request.
                return HTTP2SendDataResult::ABORT;

            case AttachmentReader::ReadStatus::ERROR_INTERNAL:
                collectSendDataResultMetric(m_metricRecorder, 1, ERROR_INTERNAL);
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
    ACSDK_DEBUG7(LX("onReceiveResponseCode").d("responseCode", responseCode));

    reportMessageRequestAcknowledged();

    if (HTTPResponseCode::CLIENT_ERROR_FORBIDDEN == intToHTTPResponseCode(responseCode)) {
        m_context->onForbidden(m_authToken);
    }

    m_responseCode = responseCode;

    // Map HTTPResponseCode values to MessageRequestObserverInterface::Status values.
    static const std::unordered_map<long, MessageRequestObserverInterface::Status> responseToResult = {
        {HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED, MessageRequestObserverInterface::Status::INTERNAL_ERROR},
        {HTTPResponseCode::SUCCESS_OK, MessageRequestObserverInterface::Status::SUCCESS},
        {HTTPResponseCode::SUCCESS_ACCEPTED, MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED},
        {HTTPResponseCode::SUCCESS_NO_CONTENT, MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT},
        {HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST, MessageRequestObserverInterface::Status::BAD_REQUEST},
        {HTTPResponseCode::CLIENT_ERROR_FORBIDDEN, MessageRequestObserverInterface::Status::INVALID_AUTH},
        {HTTPResponseCode::CLIENT_ERROR_THROTTLING_EXCEPTION, MessageRequestObserverInterface::Status::THROTTLED},
        {HTTPResponseCode::SERVER_ERROR_INTERNAL, MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2},
        {HTTPResponseCode::SERVER_UNAVAILABLE, MessageRequestObserverInterface::Status::REFUSED}};

    auto responseIterator = responseToResult.find(m_responseCode);
    if (responseIterator != responseToResult.end()) {
        m_resultStatus = responseIterator->second;
    } else {
        m_resultStatus = MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR;
    }

    ACSDK_DEBUG7(LX("responseCodeTranslated").d("responseStatus", m_resultStatus));

    m_messageRequest->responseStatusReceived(m_resultStatus);

    return true;
}

void MessageRequestHandler::onResponseFinished(HTTP2ResponseFinishedStatus status, const std::string& nonMimeBody) {
    ACSDK_DEBUG7(LX("onResponseFinished").d("status", status).d("responseCode", m_responseCode));

    if (HTTP2ResponseFinishedStatus::TIMEOUT == status) {
        m_context->onMessageRequestTimeout();
    }

    reportMessageRequestAcknowledged();
    reportMessageRequestFinished();

    if ((intToHTTPResponseCode(m_responseCode) != HTTPResponseCode::SUCCESS_OK) && !nonMimeBody.empty()) {
        m_messageRequest->exceptionReceived(nonMimeBody);
    }

    bool receivedResponseCode = MessageRequestObserverInterface::Status::PENDING != m_resultStatus;

    // Map HTTP2ResponseFinishedStatus to a MessageRequestObserverInterface::Status.

    switch (status) {
        case HTTP2ResponseFinishedStatus::COMPLETE:
            if (!receivedResponseCode) {
                m_resultStatus = MessageRequestObserverInterface::Status::INTERNAL_ERROR;
            }
            break;
        case HTTP2ResponseFinishedStatus::TIMEOUT:
            m_resultStatus = MessageRequestObserverInterface::Status::TIMEDOUT;
            break;
        case HTTP2ResponseFinishedStatus::CANCELLED:
            m_resultStatus = MessageRequestObserverInterface::Status::CANCELED;
            break;
        case HTTP2ResponseFinishedStatus::INTERNAL_ERROR:
            m_resultStatus = MessageRequestObserverInterface::Status::INTERNAL_ERROR;
            break;
        default:
            ACSDK_ERROR(LX("unhandledHTTP2ResponseFinishedStatus").d("status", status));
            m_resultStatus = MessageRequestObserverInterface::Status::INTERNAL_ERROR;
    }

    if (!receivedResponseCode) {
        m_messageRequest->responseStatusReceived(m_resultStatus);
    }

    m_messageRequest->sendCompleted(m_resultStatus);

    submitMessageSendErrorMetric(m_metricRecorder, m_resultStatus, m_messageRequest);
}

}  // namespace acl
}  // namespace alexaClientSDK

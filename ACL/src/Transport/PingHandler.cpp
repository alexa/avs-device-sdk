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

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/PingHandler.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::utils::http;
using namespace avsCommon::utils::http2;

/// URL to send pings to
const static std::string AVS_PING_URL_PATH_EXTENSION = "/ping";

/// Max time to wait for a ping reply.
static const std::chrono::milliseconds PING_TRANSFER_TIMEOUT(30000);

/// Priority for ping requests (high, default is 16).
static const uint8_t PING_PRIORITY = 200;

/// Prefix for the ID of ping requests.
static const std::string PING_ID_PREFIX = "AVSPing-";

/// String to identify log entries originating from this file.
static const std::string TAG("PingHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PingHandler> PingHandler::create(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken) {
    ACSDK_DEBUG5(LX(__func__).d("context", context.get()));

    if (!context) {
        ACSDK_CRITICAL(LX("createFailed").d("reason", "nullContext"));
        return nullptr;
    }

    if (authToken.empty()) {
        ACSDK_DEBUG9(LX("createFailed").d("reason", "emptyAuthToken"));
        return nullptr;
    }

    std::shared_ptr<PingHandler> handler(new PingHandler(context, authToken));

    HTTP2RequestConfig cfg{HTTP2RequestType::GET, context->getEndpoint() + AVS_PING_URL_PATH_EXTENSION, PING_ID_PREFIX};
    cfg.setRequestSource(handler);
    cfg.setResponseSink(handler);
    cfg.setTransferTimeout(PING_TRANSFER_TIMEOUT);
    cfg.setPriority(PING_PRIORITY);

    auto request = context->createAndSendRequest(cfg);

    if (!request) {
        ACSDK_ERROR(LX("createFailed").d("reason", "createAndSendRequestFailed"));
        return nullptr;
    }

    return handler;
}

PingHandler::PingHandler(std::shared_ptr<ExchangeHandlerContextInterface> context, const std::string& authToken) :
        ExchangeHandler{context, authToken},
        m_wasPingAcknowledgedReported{false},
        m_responseCode{0} {
    ACSDK_DEBUG5(LX(__func__).d("context", context.get()));
}

void PingHandler::reportPingAcknowledged() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_wasPingAcknowledgedReported) {
        m_wasPingAcknowledgedReported = true;
        m_context->onPingRequestAcknowledged(
            HTTPResponseCode::SUCCESS_NO_CONTENT == intToHTTPResponseCode(m_responseCode));
    }
}

std::vector<std::string> PingHandler::getRequestHeaderLines() {
    ACSDK_DEBUG5(LX(__func__));
    return {m_authHeader};
}

HTTP2SendDataResult PingHandler::onSendData(char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size));
    return HTTP2SendDataResult::COMPLETE;
}

bool PingHandler::onReceiveResponseCode(long responseCode) {
    ACSDK_DEBUG5(LX(__func__).d("responseCode", responseCode));

    if (HTTPResponseCode::CLIENT_ERROR_FORBIDDEN == intToHTTPResponseCode(responseCode)) {
        m_context->onForbidden(m_authToken);
    }

    m_context->onActivity();
    m_responseCode = responseCode;
    reportPingAcknowledged();
    return true;
}

bool PingHandler::onReceiveHeaderLine(const std::string& line) {
    ACSDK_DEBUG5(LX(__func__).d("line", line));
    m_context->onActivity();
    return true;
}

HTTP2ReceiveDataStatus PingHandler::onReceiveData(const char* bytes, size_t size) {
    ACSDK_DEBUG5(LX(__func__).d("size", size));
    m_context->onActivity();
    return HTTP2ReceiveDataStatus::SUCCESS;
}

void PingHandler::onResponseFinished(HTTP2ResponseFinishedStatus status) {
    ACSDK_DEBUG5(LX(__func__).d("status", status));
    switch (status) {
        case HTTP2ResponseFinishedStatus::COMPLETE:
            reportPingAcknowledged();
            return;
        case HTTP2ResponseFinishedStatus::TIMEOUT:
        case HTTP2ResponseFinishedStatus::INTERNAL_ERROR:
            m_context->onPingTimeout();
            return;
        case HTTP2ResponseFinishedStatus::CANCELLED:
            ACSDK_WARN(LX("onResponseFinishedWithCancelledStatus"));
            return;
    }

    ACSDK_ERROR(LX("onResponseFinishedWithUnhandledStatus").d("status", static_cast<int>(status)));
}

}  // namespace acl
}  // namespace alexaClientSDK

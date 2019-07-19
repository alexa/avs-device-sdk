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
#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/DownchannelHandler.h"
#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/MimeResponseSink.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::utils::http;
using namespace avsCommon::utils::http2;

/// Downchannel URL
const static std::string AVS_DOWNCHANNEL_URL_PATH_EXTENSION = "/v20160207/directives";

/// Prefix for the ID of downchannel requests.
static const std::string DOWNCHANNEL_ID_PREFIX = "AVSDownChannel-";

/// How long to wait for a downchannel request to connect to AVS.
static const std::chrono::seconds ESTABLISH_CONNECTION_TIMEOUT = std::chrono::seconds(60);

/// String to identify log entries originating from this file.
static const std::string TAG("DownchannelHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<DownchannelHandler> DownchannelHandler::create(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken,
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager) {
    ACSDK_DEBUG9(LX(__func__).d("context", context.get()));

    if (!context) {
        ACSDK_CRITICAL(LX("createFailed").d("reason", "nullHttp2Transport"));
        return nullptr;
    }

    if (authToken.empty()) {
        ACSDK_DEBUG9(LX("createFailed").d("reason", "emptyAuthToken"));
        return nullptr;
    }

    std::shared_ptr<DownchannelHandler> handler(new DownchannelHandler(context, authToken));

    HTTP2RequestConfig cfg{
        HTTP2RequestType::GET, context->getEndpoint() + AVS_DOWNCHANNEL_URL_PATH_EXTENSION, DOWNCHANNEL_ID_PREFIX};
    cfg.setRequestSource(handler);
    cfg.setResponseSink(std::make_shared<HTTP2MimeResponseDecoder>(
        std::make_shared<MimeResponseSink>(handler, messageConsumer, attachmentManager, cfg.getId())));
    cfg.setConnectionTimeout(ESTABLISH_CONNECTION_TIMEOUT);
    cfg.setIntermittentTransferExpected();

    auto request = context->createAndSendRequest(cfg);

    if (!request) {
        ACSDK_ERROR(LX("createFailed").d("reason", "createAndSendRequestFailed"));
        return nullptr;
    }

    return handler;
}

std::vector<std::string> DownchannelHandler::getRequestHeaderLines() {
    ACSDK_DEBUG9(LX(__func__));
    return {m_authHeader};
}

HTTP2SendDataResult DownchannelHandler::onSendData(char* bytes, size_t size) {
    ACSDK_DEBUG9(LX(__func__).d("size", size));
    return HTTP2SendDataResult::COMPLETE;
}

DownchannelHandler::DownchannelHandler(
    std::shared_ptr<ExchangeHandlerContextInterface> context,
    const std::string& authToken) :
        ExchangeHandler{context, authToken} {
    ACSDK_DEBUG9(LX(__func__).d("context", context.get()));
}

void DownchannelHandler::onActivity() {
    m_context->onActivity();
}

bool DownchannelHandler::onReceiveResponseCode(long responseCode) {
    ACSDK_DEBUG5(LX(__func__).d("responseCode", responseCode));
    switch (intToHTTPResponseCode(responseCode)) {
        case HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED:
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
        case HTTPResponseCode::SUCCESS_END_CODE:
        case HTTPResponseCode::REDIRECTION_MULTIPLE_CHOICES:
        case HTTPResponseCode::REDIRECTION_MOVED_PERMANENTLY:
        case HTTPResponseCode::REDIRECTION_FOUND:
        case HTTPResponseCode::REDIRECTION_SEE_ANOTHER:
        case HTTPResponseCode::REDIRECTION_TEMPORARY_REDIRECT:
        case HTTPResponseCode::REDIRECTION_PERMANENT_REDIRECT:
        case HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST:
        case HTTPResponseCode::SERVER_ERROR_INTERNAL:
            break;
        case HTTPResponseCode::SUCCESS_OK:
            m_context->onDownchannelConnected();
            break;
        case HTTPResponseCode::CLIENT_ERROR_FORBIDDEN:
            m_context->onForbidden(m_authToken);
            break;
    }
    return true;
}

void DownchannelHandler::onResponseFinished(HTTP2ResponseFinishedStatus status, const std::string& nonMimeBody) {
    ACSDK_DEBUG5(LX(__func__).d("status", status).d("nonMimeBody", nonMimeBody));
    m_context->onDownchannelFinished();
}

}  // namespace acl
}  // namespace alexaClientSDK

/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace rapidjson;
using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("ExceptionEncountered");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for this event
static const std::string NAMESPACE = "System";

/// JSON value for a ExceptionEncountered event's name.
static const std::string EXCEPTION_ENCOUNTERED_EVENT_NAME = "ExceptionEncountered";

/// The context key in the ExceptionEncountered event.
static const char CONTEXT_KEY_STRING[] = "context";

/// JSON key for the unparsedDirective field of the ExceptionEncountered event.
static const char UNPARSED_DIRECTIVE_KEY_STRING[] = "unparsedDirective";

/// JSON key for the error field of the ExceptionEncountered event.
static const char ERROR_KEY[] = "error";

/// JSON key for the ExceptionEncountered event's error type.
static const char ERROR_TYPE_KEY[] = "type";

/// JSON key for the ExceptionEncountered event's error message.
static const char ERROR_MESSAGE_KEY[] = "message";

std::unique_ptr<ExceptionEncounteredSender> ExceptionEncounteredSender::create(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messagesender) {
    if (!messagesender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }
    return std::unique_ptr<ExceptionEncounteredSender>(new ExceptionEncounteredSender(messagesender));
}

void ExceptionEncounteredSender::sendExceptionEncountered(
    const std::string& unparsedDirective,
    avs::ExceptionErrorType error,
    const std::string& errorDescription) {
    // Constructing Json for Context
    rapidjson::Document contextDocument(rapidjson::kObjectType);
    rapidjson::Value contextArray(rapidjson::kArrayType);
    contextDocument.AddMember(CONTEXT_KEY_STRING, contextArray, contextDocument.GetAllocator());
    rapidjson::StringBuffer contextBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> contextWriter(contextBuffer);
    contextDocument.Accept(contextWriter);
    std::string contextJson = contextBuffer.GetString();

    // Constructing Json for Payload
    rapidjson::Document payloadDataDocument(rapidjson::kObjectType);
    payloadDataDocument.AddMember(
        UNPARSED_DIRECTIVE_KEY_STRING, rapidjson::StringRef(unparsedDirective), payloadDataDocument.GetAllocator());
    rapidjson::Document errorDataDocument(rapidjson::kObjectType);
    std::ostringstream errorStringVal;
    errorStringVal << error;
    errorDataDocument.AddMember(ERROR_TYPE_KEY, errorStringVal.str(), errorDataDocument.GetAllocator());
    rapidjson::Value messageJson(rapidjson::StringRef(errorDescription));
    errorDataDocument.AddMember(ERROR_MESSAGE_KEY, messageJson, errorDataDocument.GetAllocator());
    payloadDataDocument.AddMember(ERROR_KEY, errorDataDocument, payloadDataDocument.GetAllocator());

    rapidjson::StringBuffer payloadJson;
    rapidjson::Writer<rapidjson::StringBuffer> payloadWriter(payloadJson);
    payloadDataDocument.Accept(payloadWriter);
    std::string payload = payloadJson.GetString();

    // Payload should not be empty.
    if (payload.empty()) {
        ACSDK_ERROR(LX("sendExceptionEncounteredFailed").d("reason", "payloadEmpty"));
        return;
    }
    auto msgIdAndJsonEvent =
        buildJsonEventString(NAMESPACE, EXCEPTION_ENCOUNTERED_EVENT_NAME, "", payload, contextJson);

    // msgId should not be empty
    if (msgIdAndJsonEvent.first.empty()) {
        ACSDK_ERROR(LX("sendExceptionEncounteredFailed").d("reason", "msgIdEmpty"));
        return;
    }

    // Json event should not be empty
    if (msgIdAndJsonEvent.second.empty()) {
        ACSDK_ERROR(LX("sendExceptionEncounteredFailed").d("reason", "JsonEventEmpty"));
        return;
    }
    std::shared_ptr<MessageRequest> request = std::make_shared<MessageRequest>(msgIdAndJsonEvent.second);
    m_messageSender->sendMessage(request);
}

ExceptionEncounteredSender::ExceptionEncounteredSender(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender) :
        m_messageSender{messageSender} {
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

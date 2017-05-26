/*
 * CapabilityAgent.cpp
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSUtils/Logger/LogEntry.h>
#include <AVSUtils/Logging/Logger.h>

#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"
#include "AVSCommon/AVS/CapabilityAgent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace rapidjson;
using namespace avsCommon::sdkInterfaces;
using namespace avsUtils;

/// String to identify log entries originating from this file.
static const std::string TAG("CapabilityAgent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsUtils::logger::LogEntry(TAG, event)

/**
* Builds a JSON header object. The header includes the namespace, name, message Id and an optional
* @c dialogRequestId. The message Id required for the header is a random string that is generated and added to the
* header.
*
* @param nameSpace The namespace of the event to be included in the header.
* @param eventName The name of the event to be included in the header.
* @param dialogRequestIdString The value associated with the "dialogRequestId" key.
* @param allocator The rapidjson allocator to use to build the JSON header.
* @return A header JSON object if successful else an empty object.
*/
static Document buildHeader(const std::string &nameSpace, const std::string &eventName,
        const std::string &dialogRequestIdString, rapidjson::Document::AllocatorType &allocator);

/**
* Builds a JSON event object. The event includes the header and the payload.
*
* @param nameSpace The namespace of the event to be included in the header.
* @param eventName The name of the event to be include in the header.
* @param dialogRequestIdString The value associated with the "dialogRequestId" key.
* @param payload The payload value associated with the "payload" key.
* @param allocator The rapidjson allocator to use to build the JSON header.
* @return An event JSON object if successful else an empty object.
*/
static Document buildEvent(const std::string &nameSpace, const std::string &eventName,
        const std::string &dialogRequestIdString, const std::string &payload,
        rapidjson::Document::AllocatorType &allocator);

/// The namespace key in the header of the event.
static const std::string NAMESPACE_KEY_STRING = "namespace";

/// The name key in the header of the event.
static const std::string NAME_KEY_STRING = "name";

/// The message id key in the header of the event
static const std::string MESSAGE_ID_KEY_STRING = "messageId";

/// The dialog request Id key in the header of the event.
static const std::string DIALOG_REQUEST_ID_KEY_STRING = "dialogRequestId";

/// The header key in the event.
static const std::string HEADER_KEY_STRING = "header";

/// The payload key in the event.
static const std::string PAYLOAD_KEY_STRING = "payload";

/// The context key in the event.
static const std::string CONTEXT_KEY_STRING = "context";

/// The event key.
static const std::string EVENT_KEY_STRING = "event";

std::shared_ptr<CapabilityAgent::DirectiveInfo> CapabilityAgent::createDirectiveInfo(
        std::shared_ptr<AVSDirective> directive,
        std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> result) {
    return std::make_shared<DirectiveInfo>(directive, std::move(result));
}

CapabilityAgent::CapabilityAgent(
        const std::string &nameSpace,
        std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionEncounteredSender)
        :
        m_namespace{nameSpace},
        m_exceptionEncounteredSender{exceptionEncounteredSender} {
}

CapabilityAgent::DirectiveInfo::DirectiveInfo(
        std::shared_ptr<AVSDirective> directiveIn,
        std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> resultIn)
        :
        directive{directiveIn}, result{std::move(resultIn)} {
}

void CapabilityAgent::preHandleDirective(
        std::shared_ptr <avsCommon::AVSDirective> directive,
        std::unique_ptr <DirectiveHandlerResultInterface> result) {
    std::string messageId = directive->getMessageId();
    auto info = getDirectiveInfo(messageId);
    if (info) {
        static const std::string error{"messageIdIsAlreadyInUse"};
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", error).d("messageId", messageId));
        result->setFailed(error);
        if (m_exceptionEncounteredSender) {
            m_exceptionEncounteredSender->sendExceptionEncountered(
                    directive->getUnparsedDirective(), ExceptionErrorType::INTERNAL_ERROR, error);
        }
        return;
    }
    ACSDK_DEBUG(LX("addingMessageIdToMap").d("messageId", messageId));
    info = createDirectiveInfo(directive, std::move(result));
    {
        std::lock_guard <std::mutex> lock(m_mutex);
        m_directiveInfoMap[messageId] = info;
    }
    preHandleDirective(info);
}

bool CapabilityAgent::handleDirective(const std::string &messageId) {
    auto info = getDirectiveInfo(messageId);
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "messageIdNotFound").d("messageId", messageId));
        return false;
    }
    handleDirective(info);
    return true;
}

void CapabilityAgent::cancelDirective(const std::string &messageId) {
    auto info = getDirectiveInfo(messageId);
    if (!info) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "messageIdNotFound").d("messageId", messageId));
        return;
    }
    cancelDirective(info);
}

void CapabilityAgent::onDeregistered() {
    // default no op
}

void CapabilityAgent::removeDirective(const std::string &messageId) {
    std::lock_guard <std::mutex> lock(m_mutex);
    ACSDK_DEBUG(LX("removingMessageIdFromMap").d("messageId", messageId));
    m_directiveInfoMap.erase(messageId);
}

void CapabilityAgent::onFocusChanged(FocusState) {
    // default no-op
}

void CapabilityAgent::provideState(const unsigned int) {
    // default no-op
}

void CapabilityAgent::onContextAvailable(const std::string &) {
    // default no-op
}

void CapabilityAgent::onContextFailure(const sdkInterfaces::ContextRequestError) {
    // default no-op
}

const std::string CapabilityAgent::buildJsonEventString(
        const std::string &eventName,
        const std::string &dialogRequestIdValue,
        const std::string &jsonPayloadValue,
        const std::string &jsonContext) {
    Document eventAndContext(kObjectType);
    Document::AllocatorType &allocator = eventAndContext.GetAllocator();

    if (!jsonContext.empty()) {
        Document context(kObjectType);
        // The context needs to be parsed to convert to a JSON object.
        if (context.Parse(jsonContext).HasParseError()) {
            ACSDK_DEBUG(LX("buildJsonEventStringFailed")
                    .d("reason", "parseContextFailed")
                    .sensitive("context", jsonContext));
            return "";
        }
        eventAndContext.CopyFrom(context, allocator);
    }

    Document event = buildEvent(m_namespace, eventName, dialogRequestIdValue, jsonPayloadValue, allocator);

    if (event.ObjectEmpty()) {
        ACSDK_ERROR(LX("buildJsonEventStringFailed").d("reason", "buildEventFailed"));
        return "";
    }
    eventAndContext.AddMember(StringRef(EVENT_KEY_STRING), event, allocator);

    StringBuffer eventAndContextBuf;
    Writer<StringBuffer> writer(eventAndContextBuf);
    if (!eventAndContext.Accept(writer)) {
        ACSDK_ERROR(LX("buildJsonEventStringFailed").d("reason", "StringBufferAcceptFailed"));
        return "";
    }

    return eventAndContextBuf.GetString();
}

std::shared_ptr<CapabilityAgent::DirectiveInfo> CapabilityAgent::getDirectiveInfo(const std::string& messageId) {
    std::lock_guard <std::mutex> lock(m_mutex);
    auto it = m_directiveInfoMap.find(messageId);
    if (it != m_directiveInfoMap.end()) {
        return it->second;
    }
    return nullptr;
}

Document buildHeader(const std::string &nameSpace, const std::string &eventName, const std::string &dialogRequestIdValue,
        Document::AllocatorType &allocator) {
    Document header(kObjectType);
    std::string messageId = avsCommon::utils::uuidGeneration::generateUUID();

    header.AddMember(StringRef(NAMESPACE_KEY_STRING), nameSpace, allocator);
    header.AddMember(StringRef(NAME_KEY_STRING), eventName, allocator);
    header.AddMember(StringRef(MESSAGE_ID_KEY_STRING), messageId, allocator);

    if (!dialogRequestIdValue.empty()) {
        header.AddMember(StringRef(DIALOG_REQUEST_ID_KEY_STRING), dialogRequestIdValue, allocator);
    }
    return header;
}

Document buildEvent(const std::string &nameSpace, const std::string &eventName, const std::string &dialogRequestIdValue,
        const std::string &jsonPayloadValue, Document::AllocatorType &allocator) {
    Document payload(&allocator);
    Document event(kObjectType);

    Document header = buildHeader(nameSpace, eventName, dialogRequestIdValue, allocator);

    // If the header is empty object or during parsing the payload, an error occurs, return an empty event value.
    if (header.ObjectEmpty()) {
        ACSDK_ERROR(LX("buildEventFailed").d("reason", "headerIsEmpty"));
        return event;
    }
    // Parse the payload to convert to a JSON object.
    if (!jsonPayloadValue.empty() && payload.Parse(jsonPayloadValue).HasParseError()) {
        ACSDK_ERROR(LX("buildEventFailed").d("reason", "errorParsingPayload").sensitive("payload", jsonPayloadValue));
        return event;
    }

    event.AddMember(StringRef(HEADER_KEY_STRING), header, allocator);
    if (!jsonPayloadValue.empty()) {
            event.AddMember(StringRef(PAYLOAD_KEY_STRING), payload, allocator);
    }

    return event;
}

} // namespace avs
} // namespace avsCommon
} // namespace alexaClientSDK


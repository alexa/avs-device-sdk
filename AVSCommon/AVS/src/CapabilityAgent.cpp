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

#include <AVSUtils/Logging/Logger.h>

#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"
#include "AVSCommon/AVS/CapabilityAgent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace rapidjson;
using namespace avsCommon::sdkInterfaces;
using namespace avsUtils;

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

CapabilityAgent::~CapabilityAgent() {
}

void CapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    DirectiveAndResultInterface dirAndResInterface{directive, nullptr};
    handleDirectiveImmediately(dirAndResInterface);
}

void CapabilityAgent::preHandleDirective(std::shared_ptr<avsCommon::AVSDirective> directive,
                                         std::unique_ptr<DirectiveHandlerResultInterface> result) {
    auto messageId = directive->getMessageId();
    Logger::log("Calling preHandleDirective for messageId:" + messageId);
    auto directiveAndResult = getDirectiveAndResult(messageId, directive, std::move(result));
    if (directiveAndResult) {
        preHandleDirective(directiveAndResult);
    }
}

bool CapabilityAgent::handleDirective(const std::string &messageId) {
    auto directiveAndResult = getDirectiveAndResult(messageId);
    if (!directiveAndResult) {
        return false;
    }
    Logger::log("Calling handleDirective for messageId:" + messageId);
    handleDirective(directiveAndResult);
    return true;
}

void CapabilityAgent::cancelDirective(const std::string &messageId) {
    auto directiveAndResult = getDirectiveAndResult(messageId);
    if (!directiveAndResult) {
        return;
    }
    Logger::log("Calling cancelDirective for messageId:" + messageId);
    cancelDirective(directiveAndResult);
}

void CapabilityAgent::onDeregistered() {
    // default no op
}

void CapabilityAgent::removeDirective(const std::string &messageId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Logger::log("Removing messageId:" + messageId);
    m_directiveAndResultInterfaceMap.erase(messageId);
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

CapabilityAgent::DirectiveAndResultInterface::operator bool() const {
    return directive != nullptr && resultInterface != nullptr;
}

CapabilityAgent::CapabilityAgent(const std::string &nameSpace) :
        m_namespace{nameSpace} {
}

const std::string CapabilityAgent::buildJsonEventString(const std::string &eventName,
        const std::string &dialogRequestIdValue, const std::string &jsonPayloadValue, const std::string &jsonContext) {
    Document eventAndContext(kObjectType);
    Document::AllocatorType &allocator = eventAndContext.GetAllocator();

    if (!jsonContext.empty()) {
        Document context(kObjectType);
        // The context needs to be parsed to convert to a JSON object.
        if (context.Parse(jsonContext).HasParseError()) {
#ifdef DEBUG
            Logger::log("Error parsing context:" + jsonContext);
#endif
            return "";
        }
        eventAndContext.CopyFrom(context, allocator);
    }

    Document event = buildEvent(m_namespace, eventName, dialogRequestIdValue, jsonPayloadValue, allocator);

    if (event.ObjectEmpty()) {
        Logger::log("Error building event string: Empty Event");
        return "";
    }
    eventAndContext.AddMember(StringRef(EVENT_KEY_STRING), event, allocator);

    StringBuffer eventAndContextBuf;
    Writer<StringBuffer> writer(eventAndContextBuf);
    if (!eventAndContext.Accept(writer)) {
        Logger::log("Error building event string: Accept failed");
        return "";
    }

    return eventAndContextBuf.GetString();
}

CapabilityAgent::DirectiveAndResultInterface CapabilityAgent::getDirectiveAndResult(
        const std::string& messageId,
        std::shared_ptr<avsCommon::AVSDirective> directive,
        std::unique_ptr<DirectiveHandlerResultInterface> result) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_directiveAndResultInterfaceMap.find(messageId);
    if (m_directiveAndResultInterfaceMap.end() == it) {
        if (!directive || !result) {
            Logger::log("getDirectiveAndResult failed: Cannot find messageId:" + messageId);
            return DirectiveAndResultInterface{};
        }
        DirectiveAndResultInterface dirAndResInterface{directive, std::move(result)};
        it = m_directiveAndResultInterfaceMap.insert({messageId, dirAndResInterface}).first;
    } else if (directive || result) {
        Logger::log("getDirectiveAndResult failed: messageId " + messageId + " already exists.");
        return DirectiveAndResultInterface{};
    }
    return it->second;
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
        Logger::log("Error building event string: header is empty");
        return event;
    }
    // Parse the payload to convert to a JSON object.
    if (!jsonPayloadValue.empty() && payload.Parse(jsonPayloadValue).HasParseError()) {
        Logger::log("Error building event string: Error parsing payload:" + jsonPayloadValue);
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


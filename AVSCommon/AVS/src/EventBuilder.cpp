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

#include "AVSCommon/AVS/EventBuilder.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Metrics.h"
#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace rapidjson;
using namespace utils;

/// String to identify log entries originating from this file.
static const std::string TAG("EventBuilder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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

/**
 * Builds a JSON header object. The header includes the namespace, name, message Id and an optional
 * @c dialogRequestId. The message Id required for the header is a random string that is generated and added to the
 * header.
 *
 * @param nameSpace The namespace of the event to be included in the header.
 * @param eventName The name of the event to be included in the header.
 * @param dialogRequestIdValue The value associated with the "dialogRequestId" key.
 * @param allocator The rapidjson allocator to use to build the JSON header.
 * @param messageId Will be populated with the generated messageId of the header.
 * @return A header JSON object if successful else an empty object.
 */
static Document buildHeader(
    const std::string& nameSpace,
    const std::string& eventName,
    const std::string& dialogRequestIdValue,
    Document::AllocatorType& allocator,
    std::string* messageId) {
    Document header(kObjectType);

    if (!messageId) {
        ACSDK_ERROR(LX("buildHeaderFailed").d("reason", "nullMessageId"));
        return header;
    }
    *messageId = avsCommon::utils::uuidGeneration::generateUUID();

    header.AddMember(StringRef(NAMESPACE_KEY_STRING), nameSpace, allocator);
    header.AddMember(StringRef(NAME_KEY_STRING), eventName, allocator);
    header.AddMember(StringRef(MESSAGE_ID_KEY_STRING), *messageId, allocator);

    if (!dialogRequestIdValue.empty()) {
        header.AddMember(StringRef(DIALOG_REQUEST_ID_KEY_STRING), dialogRequestIdValue, allocator);
    }
    return header;
}

/**
 * Builds a JSON event object. The event includes the header and the payload.
 *
 * @param header The header value associated with the "header" key.
 * @param jsonPayloadValue The payload value associated with the "payload" key.
 * @param allocator The rapidjson allocator to use to build the JSON header.
 * @return An event JSON object if successful else an empty object.
 */
static Document buildEvent(Document* header, const std::string& jsonPayloadValue, Document::AllocatorType& allocator) {
    Document payload(&allocator);
    Document event(kObjectType);

    if (!header || header->ObjectEmpty()) {
        ACSDK_ERROR(LX("buildEventFailed").d("reason", "headerIsNullOrEmpty"));
        return event;
    }
    // Parse the payload to convert to a JSON object. In case of an error, return an empty event value.
    if (!jsonPayloadValue.empty() && payload.Parse(jsonPayloadValue).HasParseError()) {
        ACSDK_ERROR(LX("buildEventFailed").d("reason", "errorParsingPayload").sensitive("payload", jsonPayloadValue));
        return event;
    }

    event.AddMember(StringRef(HEADER_KEY_STRING), *header, allocator);
    if (!jsonPayloadValue.empty()) {
        event.AddMember(StringRef(PAYLOAD_KEY_STRING), payload, allocator);
    }

    return event;
}

const std::pair<std::string, std::string> buildJsonEventString(
    const std::string& nameSpace,
    const std::string& eventName,
    const std::string& dialogRequestIdValue,
    const std::string& jsonPayloadValue,
    const std::string& jsonContext) {
    Document eventAndContext(kObjectType);
    Document::AllocatorType& allocator = eventAndContext.GetAllocator();
    const std::pair<std::string, std::string> emptyPair;

    if (!jsonContext.empty()) {
        Document context(kObjectType);
        // The context needs to be parsed to convert to a JSON object.
        if (context.Parse(jsonContext).HasParseError()) {
            ACSDK_DEBUG(
                LX("buildJsonEventStringFailed").d("reason", "parseContextFailed").sensitive("context", jsonContext));
            return emptyPair;
        }
        eventAndContext.CopyFrom(context, allocator);
    }

    std::string messageId;
    Document header = buildHeader(nameSpace, eventName, dialogRequestIdValue, allocator, &messageId);
    ACSDK_DEBUG(LX("buildJsonEventString").d("messageId", messageId).d("namespace", nameSpace).d("name", eventName));

    if (eventName == "SpeechStarted" || eventName == "SpeechFinished" || eventName == "Recognize") {
        ACSDK_METRIC_IDS(TAG, eventName, messageId, dialogRequestIdValue, Metrics::Location::BUILDING_MESSAGE);
    }

    Document event = buildEvent(&header, jsonPayloadValue, allocator);

    if (event.ObjectEmpty()) {
        ACSDK_ERROR(LX("buildJsonEventStringFailed").d("reason", "buildEventFailed"));
        return emptyPair;
    }

    eventAndContext.AddMember(StringRef(EVENT_KEY_STRING), event, allocator);

    StringBuffer eventAndContextBuf;
    Writer<StringBuffer> writer(eventAndContextBuf);
    if (!eventAndContext.Accept(writer)) {
        ACSDK_ERROR(LX("buildJsonEventStringFailed").d("reason", "StringBufferAcceptFailed"));
        return emptyPair;
    }

    return std::make_pair(messageId, eventAndContextBuf.GetString());
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
